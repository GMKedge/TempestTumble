#!/usr/bin/env python3
"""Paint Tempest Tumble's atlas and beep its WAVs. Run from repo root:
    python3 tools/generate_assets.py
"""
from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SPR = ROOT / "assets" / "sprites"
SFX = ROOT / "assets" / "sfx"
MUS = ROOT / "assets" / "music"
TILE = 16
COLS, ROWS = 32, 20
W, H = COLS * TILE, ROWS * TILE
ATLAS_MAP = {}


def rgb(h: str):
    h = h.lstrip("#")
    return tuple(int(h[i : i + 2], 16) for i in (0, 2, 4)) + (255,)


C = {
    "clear": (0, 0, 0, 0),
    "ink": rgb("#1a1430"),
    "skin": rgb("#f0c8a0"),
    "skin2": rgb("#c88860"),
    "gold": rgb("#f4d03f"),
    "gold2": rgb("#c9a227"),
    "steel": rgb("#c5d0e0"),
    "steel2": rgb("#7a889c"),
    "blue": rgb("#3d6ad9"),
    "blue2": rgb("#1e3a8a"),
    "storm": rgb("#6ec8ff"),
    "storm2": rgb("#3a8fd4"),
    "cloak": rgb("#2a3d8c"),
    "cloak2": rgb("#1a2460"),
    "cloak3": rgb("#5a7ae0"),
    "hair": rgb("#3a2a18"),
    "ruin": rgb("#6b7088"),
    "ruin2": rgb("#3e4458"),
    "moss": rgb("#4a7a58"),
    "cliff": rgb("#8a7a6a"),
    "cliff2": rgb("#5a4a3c"),
    "spike": rgb("#c44c6a"),
    "heart": rgb("#e85d75"),
    "white": rgb("#fff6e8"),
    "cream": rgb("#ffe9c7"),
    "shadow": rgb("#12101c"),
    "purple": rgb("#8e6bb8"),
    "wisp": rgb("#a8e0ff"),
    "golem": rgb("#7a6a58"),
    "golem2": rgb("#4a4034"),
    "coin": rgb("#f4d03f"),
    "void": rgb("#0c1020"),
    "water": rgb("#2a5088"),
    "red": rgb("#d94a3d"),
}


def tnew():
    return Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))


def px(im, x, y, color):
    if 0 <= x < im.size[0] and 0 <= y < im.size[1]:
        im.putpixel((x, y), color)


def fill(im, x, y, w, h, color):
    d = ImageDraw.Draw(im)
    d.rectangle([x, y, x + w - 1, y + h - 1], fill=color)


def dither_fill(im, x, y, w, h, a, b):
    for j in range(h):
        for i in range(w):
            px(im, x + i, y + j, a if ((x + i) + (y + j)) % 2 == 0 else b)


def outline_opaque(im, color=None):
    if color is None:
        color = C["ink"]
    w, h = im.size
    src = im.copy()
    for y in range(h):
        for x in range(w):
            if src.getpixel((x, y))[3] != 0:
                continue
            for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and src.getpixel((nx, ny))[3] > 0:
                    im.putpixel((x, y), color)
                    break


def paste_cell(img, cx, cy, tile):
    img.paste(tile, (cx * TILE, cy * TILE), tile)


def put(img, name, cx, cy, tile):
    paste_cell(img, cx, cy, tile)
    ATLAS_MAP[name] = (cx, cy)


def tile_cliff():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["cliff"])
    dither_fill(im, 0, 6, 16, 10, C["cliff"], C["cliff2"])
    fill(im, 0, 0, 16, 4, C["moss"])
    for x in (2, 7, 12):
        px(im, x, 1, C["gold"])
    outline_opaque(im)
    return im


def tile_dirt():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["cliff2"])
    dither_fill(im, 1, 1, 14, 14, C["cliff2"], C["shadow"])
    return im


def tile_stone():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["ruin"])
    fill(im, 1, 1, 6, 6, C["ruin2"])
    fill(im, 9, 8, 6, 6, C["steel2"])
    px(im, 3, 3, C["storm"])
    outline_opaque(im)
    return im


def tile_brick():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["ruin2"])
    for y in (0, 8):
        fill(im, 0, y, 16, 7, C["ruin"])
        fill(im, 0, y + 7, 16, 1, C["ink"])
    fill(im, 7, 0, 1, 7, C["ink"])
    fill(im, 4, 8, 1, 7, C["ink"])
    px(im, 2, 2, C["gold"])
    return im


def tile_wood():
    im = tnew()
    fill(im, 0, 0, 16, 16, rgb("#6b4220"))
    for x in (3, 8, 13):
        fill(im, x, 0, 1, 16, rgb("#4a2c14"))
    return im


def tile_spike():
    im = tnew()
    for i, x in enumerate((1, 5, 9, 13)):
        fill(im, x, 8 - (i % 2) * 2, 3, 8, C["spike"])
        px(im, x + 1, 6 - (i % 2) * 2, C["white"])
    return im


def tile_platform():
    im = tnew()
    fill(im, 0, 4, 16, 5, C["steel"])
    fill(im, 0, 4, 16, 2, C["gold"])
    fill(im, 1, 9, 2, 4, C["steel2"])
    fill(im, 13, 9, 2, 4, C["steel2"])
    return im


def tile_door():
    im = tnew()
    fill(im, 2, 1, 12, 15, C["blue2"])
    fill(im, 3, 2, 10, 13, C["cloak"])
    fill(im, 6, 6, 2, 4, C["gold"])
    px(im, 10, 8, C["storm"])
    outline_opaque(im)
    return im


def tile_door_open():
    im = tnew()
    fill(im, 2, 1, 12, 15, C["storm"])
    fill(im, 4, 3, 8, 12, C["void"])
    return im


def tile_bush():
    im = tnew()
    fill(im, 2, 8, 12, 6, C["moss"])
    fill(im, 4, 4, 8, 6, C["moss"])
    px(im, 6, 6, C["gold"])
    outline_opaque(im)
    return im


def tile_flower():
    im = tnew()
    fill(im, 7, 8, 2, 7, C["moss"])
    fill(im, 5, 5, 6, 5, C["storm"])
    px(im, 7, 6, C["gold"])
    return im


def tile_crate():
    im = tnew()
    fill(im, 2, 2, 12, 12, C["golem"])
    outline_opaque(im)
    fill(im, 2, 7, 12, 1, C["ink"])
    fill(im, 7, 2, 1, 12, C["ink"])
    return im


def tile_cobble():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["ruin2"])
    dither_fill(im, 0, 0, 16, 16, C["ruin"], C["ruin2"])
    return im


def tile_water():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["water"])
    for x in range(0, 16, 4):
        px(im, x, 4, C["storm"])
        px(im, x + 2, 8, C["storm2"])
    return im


def tile_banner():
    im = tnew()
    fill(im, 7, 0, 2, 4, C["steel"])
    fill(im, 4, 3, 8, 11, C["blue"])
    fill(im, 6, 5, 4, 4, C["gold"])
    return im


def spark():
    im = tnew()
    fill(im, 6, 6, 4, 4, C["storm"])
    px(im, 7, 7, C["white"])
    return im


def whitepx():
    im = tnew()
    fill(im, 0, 0, 16, 16, C["white"])
    return im


def coin(fr):
    im = tnew()
    if fr == 0:
        fill(im, 4, 4, 8, 8, C["gold"])
        fill(im, 6, 6, 4, 4, C["gold2"])
        px(im, 7, 5, C["white"])
    else:
        fill(im, 6, 4, 4, 8, C["gold"])
        fill(im, 7, 6, 2, 4, C["gold2"])
    outline_opaque(im)
    return im


def heart(full=True):
    im = tnew()
    col = C["heart"] if full else C["steel2"]
    fill(im, 3, 5, 4, 4, col)
    fill(im, 9, 5, 4, 4, col)
    fill(im, 4, 8, 8, 5, col)
    px(im, 8, 12, col)
    if full:
        px(im, 5, 6, C["white"])
    return im


def node_icon(kind):
    im = tnew()
    fill(im, 2, 2, 12, 12, C["blue2"])
    if kind == "core":
        fill(im, 5, 5, 6, 6, C["gold"])
    elif kind == "atk":
        fill(im, 7, 3, 2, 10, C["steel"])
        fill(im, 4, 10, 8, 2, C["gold"])
    elif kind == "mob":
        fill(im, 3, 7, 10, 3, C["storm"])
        fill(im, 11, 5, 3, 3, C["white"])
    elif kind == "hp":
        fill(im, 5, 6, 6, 5, C["heart"])
    else:
        fill(im, 4, 4, 8, 8, C["purple"])
    outline_opaque(im)
    return im


# --- Gale body parts (multipart puppet) ---
def cape(fr):
    im = tnew()
    sway = (-1, 0, 1)[fr % 3]
    fill(im, 4 + sway, 2, 8, 12, C["cloak"])
    fill(im, 5 + sway, 4, 6, 9, C["cloak2"])
    fill(im, 6 + sway, 3, 2, 8, C["cloak3"])
    px(im, 7 + sway, 5, C["gold"])
    outline_opaque(im)
    return im


def arm_back(fr):
    im = tnew()
    y = 4 + (0 if fr == 0 else 1)
    fill(im, 5, y, 4, 8, C["steel"])
    fill(im, 6, y + 7, 3, 3, C["skin"])
    px(im, 6, y + 1, C["gold"])
    outline_opaque(im)
    return im


def torso(fr):
    im = tnew()
    fill(im, 3, 2, 10, 12, C["steel"])
    fill(im, 4, 3, 8, 10, C["blue"])
    fill(im, 5, 4, 6, 3, C["gold"])  # chest plate highlight
    fill(im, 6, 8, 4, 5, C["steel2"])
    if fr:
        fill(im, 5, 3, 6, 2, C["storm"])
    px(im, 7, 6, C["white"])
    outline_opaque(im)
    return im


def head(fr):
    im = tnew()
    fill(im, 4, 4, 8, 8, C["skin"])
    fill(im, 4, 3, 8, 3, C["hair"])
    fill(im, 3, 4, 2, 3, C["hair"])  # side lock
    px(im, 6, 7, C["ink"])
    px(im, 10, 7, C["ink"])
    fill(im, 6, 9, 4, 1, C["skin2"])
    if fr:
        px(im, 5, 6, C["storm"])  # visor glint
    fill(im, 3, 2, 10, 2, C["steel"])  # helm brow
    px(im, 7, 2, C["gold"])
    outline_opaque(im)
    return im


def arm_front(fr):
    im = tnew()
    # 0 idle, 1 swing back, 2 lunge
    if fr == 0:
        fill(im, 6, 3, 4, 8, C["steel"])
        fill(im, 7, 10, 3, 3, C["skin"])
        px(im, 8, 4, C["gold"])
    elif fr == 1:
        fill(im, 4, 5, 8, 4, C["steel"])
        fill(im, 11, 6, 3, 3, C["skin"])
    else:
        fill(im, 3, 6, 10, 3, C["steel"])
        fill(im, 12, 5, 3, 4, C["skin"])
        fill(im, 1, 5, 4, 2, C["gold"])  # greave glint
    outline_opaque(im)
    return im


def spear(fr):
    im = tnew()
    fill(im, 2, 7, 12, 2, C["steel"])
    fill(im, 12, 6, 3, 4, C["storm"])
    fill(im, 1, 6, 3, 4, C["gold2"])
    if fr:
        px(im, 14, 7, C["white"])
    outline_opaque(im)
    return im


def leg(fr, near=True):
    im = tnew()
    x = 6 if near else 5
    if fr == 0:
        fill(im, x, 2, 4, 10, C["steel2"])
        fill(im, x - 1, 11, 6, 3, C["gold"])  # runed greaves
        px(im, x + 1, 12, C["storm"])
    elif fr == 1:
        fill(im, x + 1, 3, 4, 8, C["steel2"])
        fill(im, x + 2, 10, 5, 4, C["gold"])
        px(im, x + 3, 11, C["storm"])
    else:  # tuck
        fill(im, x, 6, 5, 6, C["steel2"])
        fill(im, x, 11, 6, 3, C["gold"])
    outline_opaque(im)
    return im


def enemy_wisp(fr):
    im = tnew()
    fill(im, 4, 4, 8, 8, C["wisp"])
    fill(im, 5, 5, 6, 6, C["storm"])
    px(im, 6, 6, C["white"])
    px(im, 9, 6, C["ink"])
    if fr:
        fill(im, 3, 3, 10, 2, C["storm2"])
    outline_opaque(im)
    return im


def enemy_wisp_glow(fr):
    im = tnew()
    fill(im, 6, 6, 4, 4, C["storm"])
    if fr:
        fill(im, 5, 5, 6, 6, C["storm2"])
    return im


def enemy_golem_legs(fr):
    im = tnew()
    fill(im, 3, 6, 4, 8, C["golem"])
    fill(im, 9, 6 + (fr % 2), 4, 8, C["golem2"])
    outline_opaque(im)
    return im


def enemy_golem_torso():
    im = tnew()
    fill(im, 3, 3, 10, 11, C["golem"])
    fill(im, 5, 5, 6, 6, C["ruin"])
    px(im, 7, 7, C["gold"])
    outline_opaque(im)
    return im


def enemy_golem_head():
    im = tnew()
    fill(im, 4, 4, 8, 8, C["golem2"])
    px(im, 6, 7, C["red"])
    px(im, 10, 7, C["red"])
    fill(im, 6, 10, 4, 1, C["ink"])
    outline_opaque(im)
    return im


def npc_trainer():
    im = tnew()
    fill(im, 5, 8, 6, 6, C["cloak"])
    fill(im, 5, 3, 6, 6, C["skin"])
    fill(im, 4, 2, 8, 3, C["gold"])
    px(im, 6, 5, C["ink"])
    px(im, 9, 5, C["ink"])
    outline_opaque(im)
    return im


def fx_slash():
    im = tnew()
    fill(im, 2, 6, 12, 3, C["storm"])
    fill(im, 8, 3, 6, 3, C["white"])
    return im


def fx_cyclone():
    im = tnew()
    fill(im, 2, 2, 12, 12, C["storm2"])
    fill(im, 5, 5, 6, 6, C["clear"])
    fill(im, 6, 6, 4, 4, C["white"])
    return im


def fx_shield():
    im = tnew()
    fill(im, 3, 2, 10, 12, C["gold"])
    fill(im, 5, 4, 6, 8, C["blue"])
    px(im, 8, 7, C["white"])
    outline_opaque(im)
    return im


GLYPHS = {
    "0": ["11111", "10001", "10001", "10001", "10001", "10001", "11111"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["11111", "00001", "00001", "11111", "10000", "10000", "11111"],
    "3": ["11111", "00001", "00001", "01111", "00001", "00001", "11111"],
    "4": ["10001", "10001", "10001", "11111", "00001", "00001", "00001"],
    "5": ["11111", "10000", "10000", "11111", "00001", "00001", "11111"],
    "6": ["11111", "10000", "10000", "11111", "10001", "10001", "11111"],
    "7": ["11111", "00001", "00010", "00100", "00100", "00100", "00100"],
    "8": ["11111", "10001", "10001", "11111", "10001", "10001", "11111"],
    "9": ["11111", "10001", "10001", "11111", "00001", "00001", "11111"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "B": ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    "C": ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01110"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    "J": ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "Q": ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "01010", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "01010", "01010"],
    "X": ["10001", "01010", "00100", "00100", "00100", "01010", "10001"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "Z": ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
    "+": ["00000", "00100", "00100", "11111", "00100", "00100", "00000"],
    "/": ["00001", "00010", "00010", "00100", "01000", "01000", "10000"],
    ":": ["00000", "00100", "00100", "00000", "00100", "00100", "00000"],
    "!": ["00100", "00100", "00100", "00100", "00100", "00000", "00100"],
    "?": ["01110", "10001", "00001", "00010", "00100", "00000", "00100"],
    "-": ["00000", "00000", "00000", "11111", "00000", "00000", "00000"],
    ".": ["00000", "00000", "00000", "00000", "00000", "00000", "00100"],
    " ": ["00000"] * 7,
}


def font_glyph(ch):
    im = tnew()
    pat = GLYPHS.get(ch.upper(), GLYPHS["?"])
    for y, row in enumerate(pat):
        for x, bit in enumerate(row):
            if bit == "1":
                px(im, 5 + x, 5 + y, C["ink"])
    for y, row in enumerate(pat):
        for x, bit in enumerate(row):
            if bit == "1":
                px(im, 5 + x, 4 + y, C["white"])
    return im


def write_atlas_header(path):
    lines = [
        "// Auto-generated by tools/generate_assets.py",
        "#pragma once",
        "#include <glm/glm.hpp>",
        "#include <string>",
        "namespace tempest {",
        "constexpr int kAtlasW = %d;" % W,
        "constexpr int kAtlasH = %d;" % H,
        "constexpr int kTile = %d;" % TILE,
        "inline glm::vec4 atlasUV(int cx, int cy, int tw=1, int th=1) {",
        "    return glm::vec4(float(cx*kTile)/kAtlasW, float(cy*kTile)/kAtlasH,",
        "                     float(tw*kTile)/kAtlasW, float(th*kTile)/kAtlasH);",
        "}",
        "struct AtlasSprite { const char* name; int cx; int cy; };",
        "inline const AtlasSprite kSprites[] = {",
    ]
    for name, (cx, cy) in sorted(ATLAS_MAP.items()):
        lines.append('    {"%s", %d, %d},' % (name, cx, cy))
    lines += [
        "    {nullptr, 0, 0}",
        "};",
        "inline glm::vec4 spriteUV(const std::string& name) {",
        "    for (int i = 0; kSprites[i].name; ++i) {",
        "        if (name == kSprites[i].name)",
        "            return atlasUV(kSprites[i].cx, kSprites[i].cy);",
        "    }",
        "    return atlasUV(0, 0);",
        "}",
        "} // namespace tempest",
        "",
    ]
    path.write_text("\n".join(lines))


def build_atlas():
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    tiles = [
        ("cliff", tile_cliff()),
        ("dirt", tile_dirt()),
        ("stone", tile_stone()),
        ("brick", tile_brick()),
        ("wood", tile_wood()),
        ("spike", tile_spike()),
        ("platform", tile_platform()),
        ("door", tile_door()),
        ("door_open", tile_door_open()),
        ("bush", tile_bush()),
        ("flower", tile_flower()),
        ("crate", tile_crate()),
        ("cobble", tile_cobble()),
        ("water", tile_water()),
        ("banner", tile_banner()),
        ("spark", spark()),
        ("whitepx", whitepx()),
        ("coin0", coin(0)),
        ("coin1", coin(1)),
        ("heart", heart(True)),
        ("heart_empty", heart(False)),
        ("slash", fx_slash()),
        ("cyclone", fx_cyclone()),
        ("shield", fx_shield()),
        ("node_core", node_icon("core")),
        ("node_atk", node_icon("atk")),
        ("node_mob", node_icon("mob")),
        ("node_hp", node_icon("hp")),
        ("node_stat", node_icon("stat")),
        ("npc_trainer", npc_trainer()),
    ]
    for i, (n, t) in enumerate(tiles):
        put(img, n, i % COLS, i // COLS, t)

    # Gale parts on rows 2-3
    put(img, "cape0", 0, 2, cape(0))
    put(img, "cape1", 1, 2, cape(1))
    put(img, "cape2", 2, 2, cape(2))
    put(img, "arm_b0", 3, 2, arm_back(0))
    put(img, "arm_b1", 4, 2, arm_back(1))
    put(img, "torso0", 5, 2, torso(0))
    put(img, "torso1", 6, 2, torso(1))
    put(img, "head0", 7, 2, head(0))
    put(img, "head1", 8, 2, head(1))
    put(img, "arm_f0", 9, 2, arm_front(0))
    put(img, "arm_f1", 10, 2, arm_front(1))
    put(img, "arm_f2", 11, 2, arm_front(2))
    put(img, "spear0", 12, 2, spear(0))
    put(img, "spear1", 13, 2, spear(1))
    put(img, "leg0", 14, 2, leg(0, True))
    put(img, "leg1", 15, 2, leg(1, True))
    put(img, "leg_tuck", 16, 2, leg(2, True))
    put(img, "leg_far0", 17, 2, leg(0, False))
    put(img, "leg_far1", 18, 2, leg(1, False))

    put(img, "wisp0", 0, 4, enemy_wisp(0))
    put(img, "wisp1", 1, 4, enemy_wisp(1))
    put(img, "wisp_glow", 2, 4, enemy_wisp_glow(0))
    put(img, "golem_legs0", 3, 4, enemy_golem_legs(0))
    put(img, "golem_legs1", 4, 4, enemy_golem_legs(1))
    put(img, "golem_torso", 5, 4, enemy_golem_torso())
    put(img, "golem_head", 6, 4, enemy_golem_head())

    special = {"/": "slashc", ":": "colon", "+": "plus", "!": "bang", "?": "q", "-": "dash", ".": "dot", " ": "sp"}
    font_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+/:!?-. "
    for i, ch in enumerate(font_chars):
        name = "f_" + (ch if ch.isalnum() else special[ch])
        put(img, name, i % COLS, 8 + i // COLS, font_glyph(ch))

    SPR.mkdir(parents=True, exist_ok=True)
    img.save(SPR / "atlas.png")
    write_atlas_header(ROOT / "include" / "Atlas.hpp")
    print("Wrote atlas", img.size, "sprites", len(ATLAS_MAP))


def write_wav(path, samples, rate=22050):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        frames = b"".join(struct.pack("<h", max(-32767, min(32767, int(s * 32767)))) for s in samples)
        w.writeframes(frames)


def tone(freq, dur, rate=22050, vol=0.35, wave="sine", attack=0.01, release=0.05):
    n = int(dur * rate)
    a = int(attack * rate)
    r = int(release * rate)
    out = []
    for i in range(n):
        t = i / rate
        if wave == "tri":
            v = 2 * abs(2 * ((t * freq) % 1) - 1) - 1
        elif wave == "square":
            v = 1.0 if (t * freq) % 1 < 0.5 else -1.0
        else:
            v = math.sin(2 * math.pi * freq * t)
        env = 1.0
        if i < a:
            env = i / max(1, a)
        if i > n - r:
            env *= max(0.0, (n - i) / max(1, r))
        out.append(v * vol * env)
    return out


def noise(dur, rate=22050, vol=0.2):
    n = int(dur * rate)
    s = 1234567
    out = []
    for i in range(n):
        s = (1103515245 * s + 12345) & 0x7FFFFFFF
        v = (s / 0x7FFFFFFF) * 2 - 1
        out.append(v * vol * (1.0 - i / n))
    return out


def build_audio():
    write_wav(SFX / "jump.wav", tone(480, 0.07) + tone(640, 0.09, vol=0.26, wave="tri"))
    write_wav(SFX / "dash.wav", tone(220, 0.05, wave="square", vol=0.18) + noise(0.08, vol=0.14) + tone(880, 0.08, vol=0.2))
    write_wav(SFX / "hit.wav", tone(200, 0.06, wave="square", vol=0.22) + noise(0.05, vol=0.12))
    write_wav(SFX / "hurt.wav", noise(0.12, vol=0.25) + tone(160, 0.08, wave="square", vol=0.15))
    write_wav(SFX / "levelup.wav", tone(392, 0.1) + tone(523, 0.1) + tone(659, 0.1) + tone(784, 0.22))
    write_wav(SFX / "select.wav", tone(720, 0.05, vol=0.2))
    write_wav(SFX / "coin.wav", tone(880, 0.06) + tone(1240, 0.1, vol=0.28))
    song = []
    for f, d in [
        (196.00, 0.22), (246.94, 0.22), (293.66, 0.22), (392.00, 0.22),
        (329.63, 0.22), (293.66, 0.22), (246.94, 0.22), (196.00, 0.44),
        (174.61, 0.22), (196.00, 0.22), (233.08, 0.22), (293.66, 0.44),
        (261.63, 0.22), (246.94, 0.22), (220.00, 0.22), (196.00, 0.44),
    ]:
        song += tone(f, d, vol=0.16, wave="tri", attack=0.01, release=0.06)
        bass = tone(f / 2, d, vol=0.07, wave="sine")
        for i in range(len(bass)):
            song[-len(bass) + i] += bass[i]
    write_wav(MUS / "storm.wav", song)
    print("Wrote wavs")


if __name__ == "__main__":
    build_atlas()
    build_audio()
