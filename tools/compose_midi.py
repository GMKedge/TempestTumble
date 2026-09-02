#!/usr/bin/env python3
"""Compose original SMF files for Tempest Tumble (runtime MIDI + SoundFont)."""
from __future__ import annotations

import os
import struct
import sys

PPQ = 480


def vlq(n: int) -> bytes:
    n = max(0, int(n))
    buf = [n & 0x7F]
    n >>= 7
    while n:
        buf.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(buf))


def meta(delta: int, typ: int, data: bytes) -> bytes:
    return vlq(delta) + bytes([0xFF, typ, len(data)]) + data


def tempo_meta(bpm: float) -> bytes:
    us = int(round(60_000_000 / bpm))
    return meta(0, 0x51, struct.pack(">I", us)[1:])


class Track:
    def __init__(self, ch: int, program: int | None, name: str):
        self.ch = ch
        self.program = program
        self.name = name
        self.events: list[tuple[int, bytes]] = []  # abs tick, payload (no delta)

    def add(self, tick: int, payload: bytes) -> None:
        self.events.append((max(0, tick), payload))

    def cc(self, tick: int, ctl: int, val: int) -> None:
        self.add(tick, bytes([0xB0 | (self.ch & 0x0F), ctl & 0x7F, val & 0x7F]))

    def prog(self, tick: int, program: int) -> None:
        self.add(tick, bytes([0xC0 | (self.ch & 0x0F), program & 0x7F]))

    def on(self, tick: int, key: int, vel: int) -> None:
        self.add(tick, bytes([0x90 | (self.ch & 0x0F), key & 0x7F, vel & 0x7F]))

    def off(self, tick: int, key: int, vel: int = 0) -> None:
        self.add(tick, bytes([0x80 | (self.ch & 0x0F), key & 0x7F, vel & 0x7F]))

    def note(self, start: int, dur: int, key: int, vel: int) -> None:
        self.on(start, key, vel)
        self.off(start + dur, key)

    def encode(self) -> bytes:
        body = meta(0, 0x03, self.name.encode("ascii"))
        if self.program is not None:
            # program + mix CC at t=0
            pass
        ev = sorted(self.events, key=lambda e: (e[0], e[1][0] & 0xF0 != 0x90))
        t = 0
        chunks = [body]
        for tick, payload in ev:
            chunks.append(vlq(tick - t) + payload)
            t = tick
        chunks.append(meta(0, 0x2F, b""))
        data = b"".join(chunks)
        return b"MTrk" + struct.pack(">I", len(data)) + data


def smf(tracks: list[Track], bpm: float) -> bytes:
    conductor = b""
    conductor += meta(0, 0x03, b"tempo")
    conductor += tempo_meta(bpm)
    conductor += meta(0, 0x58, bytes([4, 2, 24, 8]))  # 4/4
    conductor += meta(0, 0x2F, b"")
    cond = b"MTrk" + struct.pack(">I", len(conductor)) + conductor
    n = 1 + len(tracks)
    hdr = b"MThd" + struct.pack(">IHHH", 6, 1, n, PPQ)
    return hdr + cond + b"".join(tr.encode() for tr in tracks)


BAR = 4 * PPQ  # 1920


def setup_mix(tr: Track, program: int, vol: int, pan: int, expr: int = 100) -> None:
    tr.prog(0, program)
    tr.cc(0, 7, vol)
    tr.cc(0, 10, pan)
    tr.cc(0, 11, expr)


def storm() -> bytes:
    bpm = 112
    bars = 24
    end = bars * BAR
    q = PPQ
    e = PPQ // 2
    s = PPQ // 4

    strings = Track(0, 48, "strings ostinato")
    inner = Track(1, 49, "inner strings")
    brass = Track(2, 61, "brass calls")
    harp = Track(3, 46, "harp")
    bass = Track(4, 43, "contrabass")
    timp = Track(5, 47, "timpani")
    drums = Track(9, None, "perc")

    setup_mix(strings, 48, 88, 54)
    setup_mix(inner, 49, 70, 74)
    setup_mix(brass, 61, 92, 40)
    setup_mix(harp, 46, 76, 90)
    setup_mix(bass, 43, 96, 32)
    setup_mix(timp, 47, 80, 64)
    drums.cc(0, 7, 90)
    drums.cc(0, 10, 64)
    drums.cc(0, 11, 100)

    # D minor: D F G A Bb C
    ost = [57, 62, 65, 69]  # A3 D4 F4 A4
    bassline = [38, 38, 41, 43, 38, 36, 41, 43]  # D2 D2 F2 G2 D2 C2 F2 G2 per two bars (quarters of bar-pair)
    harp_arp = [62, 65, 69, 74, 69, 65]  # D4 F4 A4 D5 A4 F4
    brass_call_a = [(69, e * 3), (74, e), (72, q), (70, q)]  # A4 D5 C5 Bb4
    brass_call_b = [(65, q), (69, q), (70, e * 3), (69, e)]  # F4 A4 Bb4 A4
    inner_tones = [65, 69, 70, 69]  # F4 A4 Bb4 A4 whole-bar holds rotating

    for bar in range(bars):
        t0 = bar * BAR
        # dynamics: swell mid-phrase
        expr = 86 + int(18 * (0.5 - abs((bar % 8) / 8 - 0.5) * 2))
        strings.cc(t0, 11, expr)
        inner.cc(t0, 11, expr - 8)
        brass.cc(t0, 11, min(127, expr + 6))
        harp.cc(t0, 11, expr)

        # strings ostinato: 8th notes, slight accent on beats
        for i, k in enumerate(ost * 2):
            vel = 78 if i % 4 == 0 else 62
            strings.note(t0 + i * e, e - 8, k, vel)

        # inner voice: whole notes
        inner.note(t0, BAR - 20, inner_tones[bar % 4], 58)

        # harp 16ths, skip every 4th bar for air
        if bar % 4 != 3:
            pat = harp_arp + [62, 65]
            for i, k in enumerate(pat * 2):
                harp.note(t0 + i * s, s - 4, k, 54 + (i % 3) * 6)

        # bass: independent quarters, two-bar cell
        cell = bassline[(bar % 8)]
        # walk inside the bar
        walk = [cell, cell - 0, cell + (5 if bar % 2 == 0 else 3), cell]
        if bar % 2 == 1:
            walk = [cell, cell - 2, cell, cell + 2]
        durs = [q, q, q, q]
        acc = t0
        for k, d in zip(walk, durs):
            bass.note(acc, d - 12, k, 86 if acc == t0 else 72)
            acc += d

        # brass calls bars 4-7, 12-15, 20-23
        if (bar % 8) >= 4:
            phrase = brass_call_a if (bar // 4) % 2 == 0 else brass_call_b
            pos = t0
            for k, d in phrase:
                brass.note(pos, d - 10, k, 90)
                pos += d
            # horn fifth under last note
            brass.note(t0 + 2 * q, 2 * q - 10, 57, 70)

        # timpani: D/A on 1 and 3, roll-ish on cadences
        timp.note(t0, q, 38, 92)  # D2
        timp.note(t0 + 2 * q, e, 45, 70)  # A2
        if bar % 8 == 7:
            for i in range(8):
                timp.note(t0 + 2 * q + i * s, s - 2, 38, 60 + i * 4)

        # perc: concert bass + hats-as-wind + crash on 8
        drums.note(t0, e, 36, 96)
        drums.note(t0 + 2 * q, e, 41, 70)
        for i in range(8):
            drums.note(t0 + i * e, 20, 42, 28 if i % 2 else 40)
        if bar % 8 == 0:
            drums.note(t0, 2 * q, 49, 72)
        if bar % 8 == 7:
            drums.note(t0 + 3 * q, q, 57, 80)

    # all notes should already be off; add all-notes-off for clean loop
    for tr in (strings, inner, brass, harp, bass, timp, drums):
        tr.cc(end - 4, 123, 0)

    return smf([strings, inner, brass, harp, bass, timp, drums], bpm)


def keep() -> bytes:
    bpm = 100
    bars = 16
    end = bars * BAR
    q = PPQ
    e = PPQ // 2
    s = PPQ // 4

    choir = Track(0, 52, "choir")
    strings = Track(1, 48, "strings")
    harp = Track(2, 46, "harp")
    bass = Track(3, 42, "cello")
    horn = Track(4, 60, "horn")
    drums = Track(9, None, "perc")

    setup_mix(choir, 52, 78, 64)
    setup_mix(strings, 48, 72, 48)
    setup_mix(harp, 46, 84, 96)
    setup_mix(bass, 42, 88, 28)
    setup_mix(horn, 60, 80, 40)
    drums.cc(0, 7, 70)

    pad = [57, 60, 64, 69]  # A3 C4 E4 A4  (A minor keep)
    bassline = [33, 36, 38, 40]  # A1 C2 D2 E2

    for bar in range(bars):
        t0 = bar * BAR
        expr = 80 + (bar % 4) * 6
        choir.cc(t0, 11, expr)
        strings.cc(t0, 11, expr)
        for k in pad:
            choir.note(t0, BAR - 30, k, 50)
        # strings slow ostinato quarters
        for i, k in enumerate([64, 69, 72, 69]):
            strings.note(t0 + i * q, q - 16, k, 60)
        arp = [57, 60, 64, 69, 72, 69, 64, 60]
        for i, k in enumerate(arp * 2):
            harp.note(t0 + i * s, s - 4, k, 58)
        bass.note(t0, 2 * q - 12, bassline[bar % 4], 80)
        bass.note(t0 + 2 * q, 2 * q - 12, bassline[bar % 4] + 7, 70)
        if bar % 4 == 2:
            horn.note(t0, 2 * q, 69, 82)
            horn.note(t0 + 2 * q, 2 * q, 72, 78)
        drums.note(t0, e, 36, 70)
        if bar % 4 == 0:
            drums.note(t0, q, 49, 50)
    for tr in (choir, strings, harp, bass, horn, drums):
        tr.cc(end - 4, 123, 0)
    return smf([choir, strings, harp, bass, horn, drums], bpm)


def title() -> bytes:
    bpm = 96
    bars = 16
    end = bars * BAR
    q = PPQ
    s = PPQ // 4

    harp = Track(0, 46, "harp")
    pad = Track(1, 89, "warm pad")
    strings = Track(2, 49, "slow strings")
    bass = Track(3, 43, "bass")

    setup_mix(harp, 46, 88, 80)
    setup_mix(pad, 89, 64, 64)
    setup_mix(strings, 49, 60, 40)
    setup_mix(bass, 43, 78, 30)

    for bar in range(bars):
        t0 = bar * BAR
        pad.cc(t0, 11, 70 + (bar % 8) * 3)
        # Dm add9 pad
        for k in (50, 57, 62, 65):
            pad.note(t0, BAR - 40, k, 48)
        arp = [62, 65, 69, 72, 74, 72, 69, 65]
        for i, k in enumerate(arp * 2):
            harp.note(t0 + i * s, s - 6, k, 62)
        strings.note(t0, BAR - 30, [69, 70, 72, 74][bar % 4], 52)
        bass.note(t0, 2 * q - 20, [38, 36, 41, 38][bar % 4], 76)
        bass.note(t0 + 2 * q, 2 * q - 20, [45, 43, 48, 45][bar % 4], 64)
    for tr in (harp, pad, strings, bass):
        tr.cc(end - 4, 123, 0)
    return smf([harp, pad, strings, bass], bpm)


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    outdir = os.path.join(root, "assets", "music")
    os.makedirs(outdir, exist_ok=True)
    files = {
        "storm.mid": storm(),
        "keep.mid": keep(),
        "title.mid": title(),
    }
    for name, data in files.items():
        path = os.path.join(outdir, name)
        with open(path, "wb") as f:
            f.write(data)
        print(f"wrote {path} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
