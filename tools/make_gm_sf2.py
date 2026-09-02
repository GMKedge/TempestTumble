#!/usr/bin/env python3
"""Write an original compact General MIDI SoundFont (wavetable, CC0)."""
from __future__ import annotations

import math
import os
import struct
import sys

SR = 22050
PAD = 46


def pcm_loop(kind: str, n: int = 512) -> list[int]:
    out = []
    for i in range(n):
        ph = 2.0 * math.pi * i / n
        if kind == "sine":
            v = math.sin(ph)
        elif kind == "tri":
            v = 2.0 * abs(2.0 * (i / n) - 1.0) - 1.0
        elif kind == "sq":
            v = 0.35 * math.sin(ph)
            for h in range(3, 12, 2):
                v += 0.35 * math.sin(h * ph) / h
        elif kind == "saw":
            v = 0.0
            for h in range(1, 12):
                v += math.sin(h * ph) / h
            v *= 0.45
        else:
            v = 0.0
        out.append(max(-32767, min(32767, int(v * 28000))))
    return out


def pcm_pluck(n: int = 4096) -> list[int]:
    out = []
    for i in range(n):
        t = i / SR
        env = math.exp(-t * 7.0)
        ph = 2.0 * math.pi * 261.63 * t
        v = math.sin(ph) + 0.45 * math.sin(2 * ph) + 0.2 * math.sin(3 * ph)
        out.append(max(-32767, min(32767, int(v * env * 24000))))
    return out


def pcm_noise(n: int = 4096, decay: float = 12.0) -> list[int]:
    # deterministic "noise" so the bank is reproducible
    out = []
    x = 1
    for i in range(n):
        x = (1103515245 * x + 12345) & 0x7FFFFFFF
        nse = ((x / 0x7FFFFFFF) * 2.0 - 1.0)
        t = i / SR
        env = math.exp(-t * decay)
        tone = math.sin(2.0 * math.pi * 90.0 * t)
        v = 0.7 * nse + 0.3 * tone
        out.append(max(-32767, min(32767, int(v * env * 26000))))
    return out


def pcm_click(n: int = 512) -> list[int]:
    out = []
    x = 99
    for i in range(n):
        x = (1664525 * x + 1013904223) & 0xFFFFFFFF
        nse = ((x / 0xFFFFFFFF) * 2.0 - 1.0)
        env = math.exp(-i / 40.0)
        out.append(max(-32767, min(32767, int(nse * env * 18000))))
    return out


def pack_zstr20(s: str) -> bytes:
    b = s.encode("ascii", "replace")[:19]
    return b + b"\0" * (20 - len(b))


def tc(seconds: float) -> int:
    """Seconds to SF2 timecents, clamped."""
    if seconds <= 0.001:
        return -12000
    v = int(1200.0 * math.log(seconds, 2.0))
    return max(-12000, min(8000, v))


class SF2:
    def __init__(self) -> None:
        self.smpl: list[int] = []
        self.shdr: list[tuple] = []  # tuples of fields before terminator
        self.igens: list[tuple[int, int]] = []
        self.ibags: list[tuple[int, int]] = []
        self.insts: list[tuple[str, int]] = []
        self.pgens: list[tuple[int, int]] = []
        self.pbags: list[tuple[int, int]] = []
        self.phdrs: list[tuple] = []

    def add_sample(self, name: str, pcm: list[int], orig: int, loop: bool) -> int:
        start = len(self.smpl)
        self.smpl.extend(pcm)
        end = len(self.smpl)
        if loop:
            ls, le = start, end - 1
        else:
            ls, le = start, start
        self.smpl.extend([0] * PAD)
        idx = len(self.shdr)
        self.shdr.append((name, start, end, ls, le, SR, orig, 0, 0, 1))
        return idx

    def begin_inst(self, name: str) -> None:
        self.insts.append((name, len(self.ibags)))

    def add_zone(self, gens: list[tuple[int, int]]) -> None:
        self.ibags.append((len(self.igens), 0))
        self.igens.extend(gens)

    def end_insts(self) -> None:
        self.insts.append(("EOI", len(self.ibags)))
        self.ibags.append((len(self.igens), 0))
        self.igens.append((0, 0))  # dummy so last zone has an end
        # actually terminator igen is not required if terminator bag points at len
        # keep one dummy gen

    def add_preset(self, name: str, preset: int, bank: int, inst_index: int, gens_extra: list[tuple[int, int]] | None = None) -> None:
        self.phdrs.append((name, preset, bank, len(self.pbags)))
        self.pbags.append((len(self.pgens), 0))
        extra = gens_extra or []
        self.pgens.extend(extra)
        self.pgens.append((41, inst_index))  # instrument

    def end_presets(self) -> None:
        self.phdrs.append(("EOP", 255, 255, len(self.pbags)))
        self.pbags.append((len(self.pgens), 0))
        self.pgens.append((0, 0))

    def dumps(self) -> bytes:
        smpl = b"".join(struct.pack("<h", v) for v in self.smpl)

        def chunk(tag: bytes, payload: bytes) -> bytes:
            if len(payload) & 1:
                payload += b"\0"
            return tag + struct.pack("<I", len(payload)) + payload

        info = b"INFO"
        info += chunk(b"ifil", struct.pack("<HH", 2, 1))
        info += chunk(b"isng", b"EMU8000\0\0")
        info += chunk(b"INAM", b"Tempest GM\0\0")
        info += chunk(b"ICRD", b"2026\0\0")
        info += chunk(b"ICMT", b"Original wavetable GM bank for Tempest Tumble / Sprout and Steel. CC0-1.0.\0")
        info += chunk(b"ICOP", b"Public Domain (CC0-1.0)\0")
        info_list = b"LIST" + struct.pack("<I", len(info)) + info

        sdta = b"sdta" + chunk(b"smpl", smpl)
        sdta_list = b"LIST" + struct.pack("<I", len(sdta)) + sdta

        def shdr_rec(t) -> bytes:
            name, start, end, ls, le, sr, orig, corr, link, stype = t
            return pack_zstr20(name) + struct.pack("<IIIIIBbHH", start, end, ls, le, sr, orig, corr, link, stype)

        shdr = b"".join(shdr_rec(t) for t in self.shdr)
        shdr += pack_zstr20("EOS") + struct.pack("<IIIIIBbHH", 0, 0, 0, 0, 0, 0, 0, 0, 0)

        inst = b"".join(pack_zstr20(n) + struct.pack("<H", bag) for n, bag in self.insts)
        ibag = b"".join(struct.pack("<HH", g, m) for g, m in self.ibags)
        igen = b"".join(struct.pack("<HH", op, amt & 0xFFFF) for op, amt in self.igens)
        imod = struct.pack("<HHHHH", 0, 0, 0, 0, 0)

        phdr = b""
        for name, preset, bank, bag in self.phdrs:
            phdr += pack_zstr20(name) + struct.pack("<HHHIII", preset, bank, bag, 0, 0, 0)
        pbag = b"".join(struct.pack("<HH", g, m) for g, m in self.pbags)
        pgen = b"".join(struct.pack("<HH", op, amt & 0xFFFF) for op, amt in self.pgens)
        pmod = struct.pack("<HHHHH", 0, 0, 0, 0, 0)

        pdta = b"pdta"
        for tag, blob in (
            (b"phdr", phdr),
            (b"pbag", pbag),
            (b"pmod", pmod),
            (b"pgen", pgen),
            (b"inst", inst),
            (b"ibag", ibag),
            (b"imod", imod),
            (b"igen", igen),
            (b"shdr", shdr),
        ):
            pdta += chunk(tag, blob)
        pdta_list = b"LIST" + struct.pack("<I", len(pdta)) + pdta

        body = b"sfbk" + info_list + sdta_list + pdta_list
        return b"RIFF" + struct.pack("<I", len(body)) + body


def range_amount(lo: int, hi: int) -> int:
    return (hi << 8) | lo


def env(attack: float, decay: float, sustain_cb: int, release: float) -> list[tuple[int, int]]:
    return [
        (34, tc(attack)),
        (36, tc(decay)),
        (37, max(0, min(1440, sustain_cb))),
        (38, tc(release)),
    ]


def inst_simple(sf: SF2, name: str, sample: int, looping: bool, attack: float, decay: float, sustain: int, release: float, filt: int = 13500) -> int:
    idx = len(sf.insts)
    sf.begin_inst(name)
    gens = [(43, range_amount(0, 127))]
    gens += env(attack, decay, sustain, release)
    gens.append((8, filt))
    if looping:
        gens.append((54, 1))
    gens.append((53, sample))
    sf.add_zone(gens)
    return idx


def build() -> bytes:
    sf = SF2()
    s_saw = sf.add_sample("saw", pcm_loop("saw", 512), 60, True)
    s_sq = sf.add_sample("square", pcm_loop("sq", 512), 60, True)
    s_sine = sf.add_sample("sine", pcm_loop("sine", 512), 60, True)
    s_tri = sf.add_sample("tri", pcm_loop("tri", 512), 60, True)
    s_pluck = sf.add_sample("pluck", pcm_pluck(), 60, False)
    s_noise = sf.add_sample("noise", pcm_noise(), 60, False)
    s_click = sf.add_sample("click", pcm_click(), 60, False)
    s_timp = sf.add_sample("timp", pcm_noise(6000, 6.0), 36, False)

    # 128 melodic instruments
    names = [
        "Piano 1", "Piano 2", "Piano 3", "Honky-tonk", "E.Piano 1", "E.Piano 2", "Harpsichord", "Clav",
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        "Drawbar Organ", "Perc Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Bandoneon",
        "Nylon Guitar", "Steel Guitar", "Jazz Guitar", "Clean Guitar", "Muted Guitar", "Overdrive", "Distortion", "Harmonics",
        "Acoustic Bass", "Finger Bass", "Pick Bass", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        "Violin", "Viola", "Cello", "Contrabass", "Tremolo Str", "Pizzicato", "Harp", "Timpani",
        "Strings", "Slow Strings", "Syn Strings 1", "Syn Strings 2", "Choir Aahs", "Voice Oohs", "Syn Choir", "Orchestra Hit",
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute", "Bottle Blow", "Shakuhachi", "Whistle", "Ocarina",
        "Square Lead", "Saw Lead", "Calliope", "Chiff Lead", "Charang", "Voice Lead", "Fifth Lead", "Bass Lead",
        "New Age Pad", "Warm Pad", "Polysynth", "Choir Pad", "Bowed Pad", "Metallic Pad", "Halo Pad", "Sweep Pad",
        "Rain", "Soundtrack", "Crystal", "Atmosphere", "Brightness", "Goblins", "Echoes", "Sci-Fi",
        "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bagpipe", "Fiddle", "Shanai",
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko", "Melodic Tom", "Synth Drum", "Reverse Cym",
        "Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone", "Helicopter", "Applause", "Gunshot",
    ]

    def profile(p: int):
        # sample, loop, attack, decay, sustain, release, filter
        if p == 46:  # harp
            return s_pluck, False, 0.002, 1.2, 0, 0.6, 12000
        if p == 47:  # timpani
            return s_timp, False, 0.001, 0.8, 0, 0.9, 6000
        if 0 <= p <= 7:
            return s_sine, True, 0.004, 1.5, 200, 0.4, 9000
        if 8 <= p <= 15:
            return s_sine, True, 0.002, 0.8, 0, 0.5, 11000
        if 16 <= p <= 23:
            return s_sq, True, 0.01, 0.2, 0, 0.15, 7000
        if 24 <= p <= 31:
            return s_pluck, False, 0.003, 0.7, 0, 0.4, 10000
        if 32 <= p <= 39:
            return s_saw, True, 0.01, 0.3, 0, 0.12, 5000
        if 40 <= p <= 45:
            return s_saw, True, 0.12, 0.4, 0, 0.35, 6500
        if 48 <= p <= 55:
            return s_saw, True, 0.18, 0.5, 0, 0.45, 7000
        if 56 <= p <= 63:
            return s_saw, True, 0.04, 0.3, 0, 0.25, 8000
        if 64 <= p <= 71:
            return s_sq, True, 0.05, 0.3, 0, 0.2, 7500
        if 72 <= p <= 79:
            return s_sine, True, 0.04, 0.2, 0, 0.15, 10000
        if 80 <= p <= 87:
            return s_saw, True, 0.01, 0.2, 0, 0.15, 9000
        if 88 <= p <= 95:
            return s_saw, True, 0.25, 0.6, 0, 0.7, 5500
        if 104 <= p <= 111:
            return s_pluck, False, 0.004, 0.6, 0, 0.35, 9000
        if 112 <= p <= 119:
            return s_noise, False, 0.001, 0.4, 0, 0.3, 7000
        return s_tri, True, 0.08, 0.4, 200, 0.4, 8000

    for p, name in enumerate(names):
        smp, looping, a, d, s, r, f = profile(p)
        inst_simple(sf, name[:19], smp, looping, a, d, s, r, f)

    # drum kit: zones by MIDI key
    drum_idx = len(sf.insts)
    sf.begin_inst("Standard Kit")
    def drum_zone(lo, hi, sample, orig, attack, decay, rel, filt):
        gens = [
            (43, range_amount(lo, hi)),
            (58, orig),  # overriding root key
            *env(attack, decay, 1440, rel),
            (8, filt),
            (54, 0),
            (53, sample),
        ]
        sf.add_zone(gens)

    # GM percussion-ish mapping
    drum_zone(35, 36, s_noise, 36, 0.001, 0.5, 0.4, 4000)   # bass
    drum_zone(37, 39, s_click, 60, 0.001, 0.12, 0.08, 12000)  # snare/side
    drum_zone(40, 43, s_timp, 38, 0.001, 0.45, 0.4, 5000)     # toms / floor
    drum_zone(44, 46, s_click, 70, 0.001, 0.08, 0.06, 13000)  # hats
    drum_zone(47, 48, s_timp, 36, 0.001, 0.7, 0.6, 4500)      # mid/high tom
    drum_zone(49, 52, s_noise, 72, 0.001, 1.2, 1.0, 9000)     # crash/cym
    drum_zone(53, 59, s_click, 80, 0.001, 0.2, 0.2, 11000)
    drum_zone(60, 81, s_noise, 60, 0.001, 0.3, 0.25, 8000)
    drum_zone(82, 127, s_click, 90, 0.001, 0.1, 0.08, 12000)

    sf.end_insts()

    for p, name in enumerate(names):
        sf.add_preset(name[:19], p, 0, p)
    sf.add_preset("Standard Kit", 0, 128, drum_idx)
    sf.end_presets()
    return sf.dumps()


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(root, "assets", "music", "gm.sf2")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    data = build()
    with open(out, "wb") as f:
        f.write(data)
    print(f"wrote {out} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
