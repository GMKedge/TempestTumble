#!/usr/bin/env python3
"""Slice gale-knight-sheet.png into gameplay atlas parts.

Run from repo root: python3 tools/slice_gale_sheet.py
"""
from __future__ import annotations

import sys
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import generate_assets as ga  # noqa: E402

SHEET = ROOT / "assets" / "ref" / "gale-knight-sheet.png"
PREVIEW = ROOT / "assets" / "ref" / "gale-sliced-preview.png"
SCALE = 0.25  # ~77 px tall assembled knight (source ~308 px)
LUM_TH = 36
A_TH = 20


def is_fg_px(r, g, b, a) -> bool:
    return a >= A_TH and (r + g + b) >= LUM_TH


def chroma(im: Image.Image) -> Image.Image:
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if not is_fg_px(r, g, b, a):
                px[x, y] = (0, 0, 0, 0)
    return im


def trim(im: Image.Image) -> Image.Image:
    px = im.load()
    w, h = im.size
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        for x in range(w):
            if px[x, y][3] > 0:
                if x < minx:
                    minx = x
                if y < miny:
                    miny = y
                if x > maxx:
                    maxx = x
                if y > maxy:
                    maxy = y
    if maxx < 0:
        return im
    return im.crop((minx, miny, maxx + 1, maxy + 1))


def scale_game(im: Image.Image) -> Image.Image:
    w = max(1, int(round(im.size[0] * SCALE)))
    h = max(1, int(round(im.size[1] * SCALE)))
    return im.resize((w, h), Image.Resampling.BOX)


def connected_components(im: Image.Image, min_n=200):
    px = im.load()
    w, h = im.size
    seen = [[False] * w for _ in range(h)]
    comps = []
    for y in range(h):
        for x in range(w):
            if seen[y][x]:
                continue
            r, g, b, a = px[x, y]
            if not is_fg_px(r, g, b, a):
                seen[y][x] = True
                continue
            q = deque([(x, y)])
            seen[y][x] = True
            minx = maxx = x
            miny = maxy = y
            n = 0
            gold = steel = blue = brown = 0
            while q:
                cx, cy = q.popleft()
                n += 1
                cr, cg, cb, ca = px[cx, cy]
                if cr > cg + 20 and cg > cb and cr > 80:
                    gold += 1
                if cb > cr + 10 and cb > 60:
                    blue += 1
                if abs(cr - cg) < 15 and abs(cg - cb) < 15 and cr > 80:
                    steel += 1
                if cr > cg + 10 and cg > cb + 10 and 60 < cr < 180:
                    brown += 1
                if cx < minx:
                    minx = cx
                if cx > maxx:
                    maxx = cx
                if cy < miny:
                    miny = cy
                if cy > maxy:
                    maxy = cy
                for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h and not seen[ny][nx]:
                        nr, ng, nb, na = px[nx, ny]
                        if is_fg_px(nr, ng, nb, na):
                            seen[ny][nx] = True
                            q.append((nx, ny))
                        else:
                            seen[ny][nx] = True
            bw = maxx - minx + 1
            bh = maxy - miny + 1
            if n < min_n:
                continue
            crop = chroma(im.crop((minx, miny, maxx + 1, maxy + 1)))
            comps.append(
                {
                    "x": minx,
                    "y": miny,
                    "w": bw,
                    "h": bh,
                    "n": n,
                    "gold": gold,
                    "steel": steel,
                    "blue": blue,
                    "brown": brown,
                    "im": crop,
                }
            )
    comps.sort(key=lambda c: (c["y"], c["x"]))
    return comps


def classify(c) -> str:
    w, h, n = c["w"], c["h"], c["n"]
    x, y = c["x"], c["y"]
    ar = w / max(1, h)
    gold, blue, brown = c["gold"], c["blue"], c["brown"]
    if h >= 200 and w >= 90:
        return "composite"
    if ar > 1.15 and h < 145:
        return "sword"
    if ar < 0.40 and h > 90:
        return "sword"
    if ar < 0.55 and w < 95 and h > 130 and gold > 200 and brown > 150:
        return "sword"
    if y < 330 and 70 < w < 145 and 90 < h < 175 and gold < 500:
        return "helmet"
    if 450 <= y <= 660 and 65 < w < 105 and 125 < h < 170 and gold < 200:
        return "leg"
    if y > 780 and 65 < w < 100 and 125 < h < 165 and gold < 900 and ar < 0.75:
        return "leg"
    if 140 < h < 175 and 75 < w < 140 and gold > 700:
        return "torso"
    if 80 < w < 100 and 90 < h < 110 and y > 600:
        return "torso"
    if w < 40 and h < 90:
        return "junk"
    if n < 400:
        return "junk"
    return "misc"


def rotate_about(im: Image.Image, angle: float, pivot=(0.50, 0.10)) -> Image.Image:
    w, h = im.size
    pad = int(max(w, h) * 0.7) + 4
    canvas = Image.new("RGBA", (w + 2 * pad, h + 2 * pad), (0, 0, 0, 0))
    canvas.paste(im, (pad, pad), im)
    cx = pad + w * pivot[0]
    cy = pad + h * pivot[1]
    rot = canvas.rotate(-angle, resample=Image.Resampling.NEAREST, center=(cx, cy))
    return trim(rot)


def offset_copy(im: Image.Image, dx: int, dy: int) -> Image.Image:
    w, h = im.size
    pad = max(abs(dx), abs(dy)) + 2
    canvas = Image.new("RGBA", (w + 2 * pad, h + 2 * pad), (0, 0, 0, 0))
    canvas.paste(im, (pad + dx, pad + dy), im)
    return trim(canvas)


def pack_items(atlas: Image.Image, items: list[tuple[str, Image.Image]], start_y: int) -> Image.Image:
    x, y, row_h = 2, start_y, 0
    W, H = atlas.size
    for name, im in items:
        w, h = im.size
        if x + w + 2 > W:
            x = 2
            y += row_h + 2
            row_h = 0
        if y + h + 2 > H:
            nh = max(H * 2, y + h + 16)
            grown = Image.new("RGBA", (W, nh), (0, 0, 0, 0))
            grown.paste(atlas, (0, 0))
            atlas = grown
            H = nh
        atlas.paste(im, (x, y), im)
        ga.ATLAS_MAP[name] = (x, y, w, h)
        x += w + 2
        row_h = max(row_h, h)
    return atlas


def crop_frac(im: Image.Image, l, t, r, b) -> Image.Image:
    w, h = im.size
    box = (int(w * l), int(h * t), int(w * r), int(h * b))
    return trim(im.crop(box))


def extract_blue_cape(im: Image.Image) -> Image.Image:
    src = im.convert("RGBA")
    px = src.load()
    w, h = src.size
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    op = out.load()
    for y in range(int(h * 0.25), h):
        for x in range(0, int(w * 0.55)):
            r, g, b, a = px[x, y]
            if a < 20:
                continue
            if b > r + 8 and b > g and b > 40:
                op[x, y] = (r, g, b, a)
            elif r < 50 and g < 50 and b < 70 and a > 80:
                op[x, y] = (r, g, b, a)
    return trim(out)


def main() -> int:
    if not SHEET.exists():
        print("missing sheet", SHEET)
        return 1
    raw = Image.open(SHEET).convert("RGBA")
    sheet = chroma(raw)
    comps = connected_components(sheet)
    counts = {}
    for c in comps:
        c["cls"] = classify(c)
        counts[c["cls"]] = counts.get(c["cls"], 0) + 1
        print(
            f"bbox ({c['x']:3d},{c['y']:3d}) {c['w']:3d}x{c['h']:3d} n={c['n']:5d} "
            f"-> {c['cls']:10s} gold={c['gold']} blue={c['blue']} brown={c['brown']}"
        )
    print("class counts:", counts)

    helmets = [c for c in comps if c["cls"] == "helmet"]
    legs = [c for c in comps if c["cls"] == "leg"]
    swords = [c for c in comps if c["cls"] == "sword"]
    torsos = [c for c in comps if c["cls"] == "torso"]
    composites = [c for c in comps if c["cls"] == "composite"]
    misc = [c for c in comps if c["cls"] == "misc"]

    # Keep 3 helmets: idle (left/top profile), look-down (shorter / lower visor), 3/4 (right side).
    helmets_sorted = sorted(helmets, key=lambda c: (c["x"], c["y"]))
    idle = helmets_sorted[0] if helmets_sorted else None
    look_down = min(helmets, key=lambda c: c["h"]) if helmets else None
    three_q = max(helmets, key=lambda c: c["x"]) if helmets else None
    chosen_h = []
    for h in (idle, look_down, three_q):
        if h is not None and all(h is not x for x in chosen_h):
            chosen_h.append(h)
    while len(chosen_h) < min(3, len(helmets)):
        for h in helmets:
            if all(h is not x for x in chosen_h):
                chosen_h.append(h)
                break
    print("kept helmets:", [(c["x"], c["y"], c["w"], c["h"]) for c in chosen_h])

    # Prefer unique-looking legs (spread in x).
    legs_sorted = sorted(legs, key=lambda c: c["x"])
    print("legs:", [(c["x"], c["y"], c["w"], c["h"]) for c in legs_sorted])

    # Torso: crop upper half of a headless composite, else classified torso.
    torso_src = None
    for c in composites:
        # headless-ish: not the tallest with a helmet (bottom-right assembled are ~308 and include heads)
        if c["y"] < 500 and c["h"] > 250:
            torso_src = crop_frac(c["im"], 0.0, 0.0, 1.0, 0.50)
            break
    if torso_src is None and composites:
        torso_src = crop_frac(composites[0]["im"], 0.05, 0.12, 0.95, 0.52)
    if torso_src is None and torsos:
        torso_src = torsos[0]["im"]
    torso1 = None
    if len(composites) > 1:
        torso1 = crop_frac(composites[1]["im"], 0.0, 0.0, 1.0, 0.48)

    # Arms from armed composites (torso+arm+leg, typically left-middle).
    arm_src = None
    for c in composites:
        if c["x"] < 200 and c["h"] > 250:
            arm_src = crop_frac(c["im"], 0.42, 0.02, 1.0, 0.46)
            break
    if arm_src is None:
        for c in misc:
            if 0.45 < c["w"] / max(1, c["h"]) < 0.7 and c["h"] > 120:
                arm_src = c["im"]
                break
    if arm_src is None and composites:
        arm_src = crop_frac(composites[0]["im"], 0.50, 0.05, 1.0, 0.45)
    # Back arm: flip a slightly smaller crop.
    arm_back_src = None
    if arm_src is not None:
        arm_back_src = arm_src.transpose(Image.Transpose.FLIP_LEFT_RIGHT)

    cape_src = None
    if composites:
        # Prefer a full assembled pose (bottom of sheet).
        assembled = [c for c in composites if c["y"] > 600] or composites
        cape_src = extract_blue_cape(assembled[0]["im"])
        if cape_src.size[0] < 8:
            cape_src = crop_frac(assembled[0]["im"], 0.0, 0.35, 0.45, 0.95)

    # Swords: pick a vertical idle and a more horizontal thrust source.
    swords_v = sorted(swords, key=lambda c: c["w"] / max(1, c["h"]))
    sword_idle = swords_v[0]["im"] if swords_v else None
    sword_wide = max(swords, key=lambda c: c["w"])["im"] if swords else None

    items: list[tuple[str, Image.Image]] = []

    def add(name, im):
        if im is None:
            print("skip empty", name)
            return
        g = scale_game(trim(im))
        if g.size[0] < 2 or g.size[1] < 2:
            print("skip tiny", name, g.size)
            return
        items.append((name, g))
        print(f"  part {name:18s} {g.size[0]:3d}x{g.size[1]:3d}  (src {im.size[0]}x{im.size[1]})")

    for i, h in enumerate(chosen_h[:3]):
        add(f"gale_head{i}", h["im"])
    add("gale_torso0", torso_src)
    add("gale_torso1", torso1 if torso1 is not None else torso_src)

    # Legs: 4-6 walk frames from sheet legs + synthesized hip rotations.
    leg_base = [c["im"] for c in legs_sorted[:3]]
    if not leg_base and composites:
        leg_base = [crop_frac(composites[0]["im"], 0.25, 0.50, 0.85, 1.0)]
    while len(leg_base) < 1:
        print("no legs found")
        break
    walk_angles = (-16, -8, 2, 12, 6, -4)
    for i, ang in enumerate(walk_angles):
        src = leg_base[i % len(leg_base)]
        fr = rotate_about(src, ang, pivot=(0.50, 0.08))
        if i in (1, 4):
            fr = offset_copy(fr, 3 if i == 1 else -2, -2)
        add(f"gale_legL{i}", fr)
        add(f"gale_legR{i}", fr.transpose(Image.Transpose.FLIP_LEFT_RIGHT) if i % 2 else offset_copy(fr, -1, 0))
    tuck = rotate_about(leg_base[0], -28, pivot=(0.5, 0.08))
    tuck = offset_copy(tuck, 0, 4)
    add("gale_leg_tuck", tuck)

    arm_angles = (6, -22, 55, 18)  # idle, backswing, thrust, recover
    if arm_src is None:
        arm_src = Image.new("RGBA", (40, 60), (0, 0, 0, 0))
    for i, ang in enumerate(arm_angles):
        fr = rotate_about(arm_src, ang, pivot=(0.35, 0.12))
        add(f"gale_armF{i}", fr)
        bsrc = arm_back_src if arm_back_src is not None else arm_src
        add(f"gale_armB{i}", rotate_about(bsrc, -ang * 0.7, pivot=(0.65, 0.12)))

    if sword_idle is not None:
        add("gale_sword0", sword_idle)
        add("gale_sword1", rotate_about(sword_idle, 40, pivot=(0.5, 0.2)))
        thrust = sword_wide if sword_wide is not None else rotate_about(sword_idle, 90, pivot=(0.5, 0.2))
        add("gale_sword_thrust", thrust)
    else:
        print("no sword")

    if cape_src is not None:
        add("gale_cape0", cape_src)
        add("gale_cape1", offset_copy(cape_src, 3, 1))
        add("gale_cape2", offset_copy(cape_src, -4, 2))
    else:
        print("no cape")

    # Aliases so leftover code still resolves.
    alias_src = {n: im for n, im in items}
    aliases = [
        ("head0", "gale_head0"),
        ("head1", "gale_head1" if "gale_head1" in alias_src else "gale_head0"),
        ("torso0", "gale_torso0"),
        ("torso1", "gale_torso1" if "gale_torso1" in alias_src else "gale_torso0"),
        ("leg0", "gale_legL0"),
        ("leg1", "gale_legL2"),
        ("leg_tuck", "gale_leg_tuck"),
        ("leg_far0", "gale_legR0"),
        ("leg_far1", "gale_legR2"),
        ("arm_f0", "gale_armF0"),
        ("arm_f1", "gale_armF1"),
        ("arm_f2", "gale_armF2"),
        ("arm_b0", "gale_armB0"),
        ("arm_b1", "gale_armB1"),
        ("cape0", "gale_cape0"),
        ("cape1", "gale_cape1"),
        ("cape2", "gale_cape2"),
        ("spear0", "gale_sword0"),
        ("spear1", "gale_sword_thrust"),
    ]

    # Debug labeled preview of source classification + kept crops.
    preview = Image.new("RGBA", (raw.size[0] + 420, raw.size[1]), (12, 12, 16, 255))
    preview.paste(raw, (0, 0))
    d = ImageDraw.Draw(preview)
    colors = {
        "helmet": (255, 220, 80),
        "leg": (80, 220, 120),
        "sword": (255, 120, 80),
        "torso": (80, 180, 255),
        "composite": (220, 80, 220),
        "misc": (180, 180, 180),
        "junk": (80, 80, 80),
    }
    for i, c in enumerate(comps):
        col = colors.get(c["cls"], (255, 255, 255))
        x, y, w, h = c["x"], c["y"], c["w"], c["h"]
        d.rectangle([x, y, x + w - 1, y + h - 1], outline=col + (255,))
        d.text((x + 2, y + 2), f"{i}:{c['cls'][:4]}", fill=col + (255,))
    py = 8
    d.text((raw.size[0] + 8, py), "kept gameplay parts (scaled)", fill=(255, 255, 255, 255))
    py += 18
    for name, im in items:
        if py + im.size[1] > preview.size[1] - 8:
            break
        preview.paste(im, (raw.size[0] + 8, py), im)
        d.text((raw.size[0] + 16 + im.size[0], py), f"{name} {im.size[0]}x{im.size[1]}", fill=(230, 230, 230, 255))
        py += im.size[1] + 6
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    preview.save(PREVIEW)
    print("wrote preview", PREVIEW)

    # Pack tiles/UI/font/enemies then gale parts.
    ga.ATLAS_MAP.clear()
    atlas = ga.build_atlas(include_gale=False)
    # build_atlas already saved a tiles-only atlas; we overwrite after packing gale.
    start_y = 6 * ga.TILE  # below tile row 0 and enemy row 4
    # Don't overlap enemies at row 4 (y=64, 32px tall -> y=96). Font is at row 8 (y=128).
    # Pack gale into unused rows 2-3 (y=32) and after font if needed.
    # Enemies occupy cx 11-25 at cy 4. Font at cy 8-9.
    # Use y=32 (row 2) which is empty when include_gale=False.
    atlas = pack_items(atlas, items, start_y=10 * ga.TILE + 4)
    # Aliases share the same atlas rect.
    for alias, src in aliases:
        if src in ga.ATLAS_MAP and alias not in ga.ATLAS_MAP:
            ga.ATLAS_MAP[alias] = ga.ATLAS_MAP[src]

    ga.SPR.mkdir(parents=True, exist_ok=True)
    atlas.save(ga.SPR / "atlas.png")
    ga.W, ga.H = atlas.size
    ga.write_atlas_header(ga.ROOT / "include" / "Atlas.hpp", atlas.size[0], atlas.size[1])
    print("Wrote atlas", atlas.size, "sprites", len(ga.ATLAS_MAP))
    gale_names = [n for n, _ in items]
    print("gale parts:", ", ".join(gale_names))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
