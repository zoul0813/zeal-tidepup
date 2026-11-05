#!/usr/bin/env python3
"""Generate the Tidepup tileset (assets/tileset.gif) and its tile index header (src/gfx.h).

All art is text pixel art so it stays diffable. One 256-tile, 8-bit tileset is shared by both
layers and all sprites. Palette index 0 is transparent on layer 1 and on sprites; on layer 0 it is
the top band of the sky.

Layout:
    0         empty (air)
    1..31     shore and seabed tiles (layer 1)
    32..95    font at the ASCII codes
    96..127   sky and sea backdrop (layer 0)
    128..     sprites. The pup's vertical (leaping / diving) frames and the buoys and the mother
              seal are hardware 16x32 sprites: tile N is the top half, N+1 the bottom half.
Run: python3 tools/make_assets.py   (from the game folder, needs Pillow)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import Tileset  # noqa: E402

PAL = [
    ('0', (110, 184, 228)),  # 0 sky top / transparent
    ('1', (140, 200, 236)),
    ('2', (176, 218, 242)),
    ('3', (210, 234, 246)),  # horizon
    ('4', (236, 246, 252)),  # far iceberg lit
    ('5', (190, 216, 232)),  # far iceberg shade
    ('6', (64, 146, 198)),   # water just under the surface
    ('7', (44, 118, 174)),
    ('8', (30, 94, 150)),
    ('9', (22, 72, 124)),
    ('A', (14, 52, 98)),
    ('B', (8, 36, 72)),      # deepest
    ('C', (214, 244, 255)),  # surface shimmer (palette cycled)
    ('D', (130, 204, 236)),  # surface shimmer 2 (palette cycled)
    ('E', (96, 172, 214)),   # light ray
    ('k', (18, 22, 34)),
    ('w', (255, 255, 255)),
    # ice
    ('i', (226, 242, 250)),
    ('j', (184, 218, 236)),
    ('l', (134, 182, 212)),
    ('s', (250, 252, 255)),  # snow
    ('g', (160, 230, 250)),  # slick ice gloss
    # rock
    ('r', (104, 110, 124)),
    ('R', (72, 78, 92)),
    ('q', (46, 50, 62)),
    # sand
    ('y', (204, 184, 126)),
    ('Y', (160, 138, 90)),
    # kelp / coral
    ('G', (70, 160, 86)),
    ('h', (40, 108, 60)),
    ('c', (240, 110, 120)),
    # urchin
    ('u', (64, 34, 90)),
    ('U', (140, 92, 176)),
    # pup (white coat)
    ('P', (246, 248, 252)),
    ('p', (204, 214, 226)),
    ('o', (150, 162, 180)),
    # adult seal
    ('S', (124, 134, 152)),
    ('t', (90, 98, 116)),
    ('T', (176, 186, 200)),
    # fish
    ('F', (255, 168, 52)),
    ('f', (214, 108, 30)),
    ('e', (255, 226, 140)),
    # crab
    ('K', (232, 76, 52)),
    ('n', (150, 40, 32)),
    # gull
    ('W', (236, 238, 242)),
    ('m', (150, 156, 170)),
    ('a', (244, 168, 56)),   # beak / feet
    # jelly
    ('J', (255, 150, 206)),
    ('x', (210, 96, 170)),
    # buoy, hearts, ui
    ('H', (232, 52, 72)),
    ('z', (150, 24, 44)),
    ('b', (190, 236, 252)),  # bubble
    ('v', (90, 220, 120)),   # green flag
]
ts = Tileset(PAL)
grid, put = ts.grid, ts.put
WATER = 'TF_USER1'
SLICK = 'TF_USER2'

# ---------------------------------------------------------------- layer 1: shore and seabed
put(0, 'TILE_EMPTY', ts.blank())
put(1, 'TILE_WATER', ts.blank(), [WATER])   # see-through: the backdrop shows the sea


def rock_body(seed):
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            v = (x * 7 + y * 13 + seed * 5 + (x * y) % 7) % 23
            r += 'q' if v == 0 else ('r' if v in (3, 11) else 'R')
        rows.append(r)
    return rows


def top_edge(rows, light, mid, dark, left=False, right=False):
    rows = [list(r) for r in rows]
    for x in range(16):
        rows[0][x] = 'k'
        rows[1][x] = light
        rows[2][x] = light if x % 5 else mid
        rows[3][x] = mid if (x + 2) % 4 else rows[3][x]
    if left:
        for y in range(16):
            rows[y][0] = 'k'
            rows[y][1] = mid if y > 1 else rows[y][1]
        rows[0][0] = '.'
        rows[1][0] = '.'
        rows[0][1] = '.'
        rows[1][1] = 'k'
    if right:
        for y in range(16):
            rows[y][15] = 'k'
            rows[y][14] = dark if y > 1 else rows[y][14]
        rows[0][15] = '.'
        rows[1][15] = '.'
        rows[0][14] = '.'
        rows[1][14] = 'k'
    return [''.join(r) for r in rows]


rock = rock_body(1)
put(2, 'TILE_ROCK_TOP_L', grid(top_edge(rock, 'r', 'R', 'q', left=True)), ['TF_SOLID'])
put(3, 'TILE_ROCK_TOP', grid(top_edge(rock, 'r', 'R', 'q')), ['TF_SOLID'])
put(4, 'TILE_ROCK_TOP_R', grid(top_edge(rock, 'r', 'R', 'q', right=True)), ['TF_SOLID'])
put(5, 'TILE_ROCK', grid(rock_body(2)), ['TF_SOLID'])


def ice_body(seed):
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            v = (x * 3 + y * 5 + seed) % 19
            r += 'l' if (v == 0 or (x + seed) % 16 == 15) else ('j' if v in (4, 9) or y % 8 == 7 else 'i')
        rows.append(r)
    return rows


def snow_cap(rows, left=False, right=False):
    rows = [list(r) for r in rows]
    for x in range(16):
        rows[0][x] = 'k'
        rows[1][x] = 's'
        rows[2][x] = 's'
        rows[3][x] = 's' if (x * 5) % 7 > 1 else 'j'
        rows[4][x] = 's' if (x * 3) % 8 == 2 else rows[4][x]
    if left:
        for y in range(16):
            rows[y][0] = 'k'
        rows[0][0] = '.'
        rows[0][1] = '.'
        rows[1][0] = '.'
        rows[1][1] = 'k'
    if right:
        for y in range(16):
            rows[y][15] = 'k'
            rows[y][14] = 'l' if y > 4 else rows[y][14]
        rows[0][15] = '.'
        rows[0][14] = '.'
        rows[1][15] = '.'
        rows[1][14] = 'k'
    return [''.join(r) for r in rows]


ice = ice_body(3)
put(6, 'TILE_ICE_TOP_L', grid(snow_cap(ice, left=True)), ['TF_SOLID'])
put(7, 'TILE_ICE_TOP', grid(snow_cap(ice)), ['TF_SOLID'])
put(8, 'TILE_ICE_TOP_R', grid(snow_cap(ice, right=True)), ['TF_SOLID'])
put(9, 'TILE_ICE', grid(ice_body(7)), ['TF_SOLID'])
slick = [list(r) for r in ice_body(5)]
for x in range(16):
    slick[0][x] = 'k'
    slick[1][x] = 'w'
    slick[2][x] = 'g'
    slick[3][x] = 'g' if x % 6 else 'w'
for x in (3, 4, 10):
    slick[2][x] = 'w'
put(10, 'TILE_SLICK', grid([''.join(r) for r in slick]), ['TF_SOLID', SLICK])


def sand(top):
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            v = (x * 11 + y * 7) % 13
            r += 'Y' if v == 0 or (y > 10 and v < 3) else 'y'
        rows.append(r)
    if top:
        rows = [list(r) for r in rows]
        for x in range(16):
            rows[0][x] = '.' if x % 7 in (2, 3) else 'k'
            rows[1][x] = 'k' if x % 7 in (2, 3) else 'y'
        rows = [''.join(r) for r in rows]
    return rows


# seabed tiles sit under water: their transparent pixels show the sea, so they keep the water flag off
put(11, 'TILE_SAND_TOP', grid(sand(True)), ['TF_SOLID'])
put(12, 'TILE_SAND', grid(sand(False)), ['TF_SOLID'])

urchin = [
    "", "", "", "",
    "..u...u...u.....",
    "...u..u..u......",
    ".u..u.u.u..u....",
    "..u.uuuuu.u.....",
    "uu.uuUUUuu.uu...",
    "...uUUwUUu......",
    "uuuuUUUUUuuuu...",
    "...uuUUUuu......",
    ".uu.uuuuu.uu....",
    "..u.u.u.u.u.....",
    ".u..u.u.u..u....",
    "yyyyyyyyyyyyyyyy",
]
urchin = [r[:13].rjust(14, '.').ljust(16, '.') if i < 15 else r for i, r in enumerate(urchin)]
put(13, 'TILE_URCHIN', grid(urchin), [WATER, 'TF_TRIGGER'])
kelp_top = [
    "......G.........",
    ".....GhG........",
    ".....Gh.........",
    "......hG........",
    "......Gh..G.....",
    "......hG.GhG....",
    ".....Gh..Gh.....",
    ".....hG...hG....",
    "......Gh..Gh....",
    "......hG.Gh.....",
    ".....Gh..hG.....",
    ".....hG...Gh....",
    "......Gh..hG....",
    "......hG.Gh.....",
    ".....Gh..hG.....",
    ".....hG...Gh....",
]
put(14, 'TILE_KELP_TOP', grid(kelp_top), [WATER])
kelp = [kelp_top[(i + 6) % 16].replace('.....GhG', '......hG') for i in range(16)]
kelp = [r if i > 2 else "......Gh..hG...." for i, r in enumerate(kelp)]
put(15, 'TILE_KELP', grid(kelp), [WATER])
coral = [
    "", "", "", "", "", "",
    "....c.....c.....",
    "...ccc...cc.....",
    "....c.c.cc..c...",
    "..c.ccc.c..ccc..",
    ".ccc.ccccc..c...",
    "..c...ccc..cc...",
    "......ccc.cc....",
    ".......cccc.....",
    "yyyyyyyyyyyyyyyy",
    "yYyyyyyyyYyyyyyy",
]
put(16, 'TILE_CORAL', grid(coral), [WATER])
# an ice floe floating in the surface row: stand on it from above, swim up through it
floe = ["kkkkkkkkkkkkkkkk",
        "ssssssssssssssss",
        "isiiiissiiiisiii",
        "iiijiiiiiijiiiii",
        "kjjjjjjjjjjjjjjk",
        ".kkkkkkkkkkkkkk."]
put(17, 'TILE_FLOE', grid(floe), ['TF_ONEWAY', WATER])   # sits in the surface row
# ice underwater: same as ice, but a cave ceiling of ice (the seal can't surface through it)
icicles = [list(r) for r in ice_body(9)]
for y in range(12, 16):
    for x in range(16):
        if not (x % 5 in (1, 2) and y < 12 + (x % 3) + 1):
            icicles[y][x] = '.'
for x in range(16):
    if icicles[11][x] != '.':
        icicles[11][x] = 'l'
put(18, 'TILE_ICICLES', grid([''.join(r) for r in icicles]), ['TF_SOLID'])
shell = ["", "", "", "", "", "", "", "", "", "",
         "......kkkk......",
         ".....kcwcck.....",
         "....kcccwcck....",
         "....kkkkkkkk....",
         "yyyyyyyyyyyyyyyy",
         "yyyYyyyyyyyyYyyy"]
put(19, 'TILE_SHELL', grid(shell), [WATER])
# snow tuft on land, decoration only
tuft = ["", "", "", "", "", "", "", "", "", "",
        "", "", "....k......k....", "...ksk....ksk...", "..ksssk..kssk...", ".kssssskkssssk.."]
put(20, 'TILE_TUFT', grid(tuft))

ts.font()

# ---------------------------------------------------------------- layer 0: sky and sea (opaque)


def band(a, b=None, pattern=0):
    """solid color a, or a dither of a and b"""
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            if b is None:
                r += a
            elif pattern == 0:      # top half a, dithered middle, bottom half b
                if y < 5:
                    r += a
                elif y > 10:
                    r += b
                else:
                    r += b if (x + y) % 2 == 0 and (y > 7 or (x + y) % 4 == 0) else a
            else:
                r += b if (x + y) % 2 == 0 else a
        rows.append(r)
    return grid(rows)


put(96, 'BG_SKY0', band('0'))
put(97, 'BG_SKY01', band('0', '1'))
put(98, 'BG_SKY1', band('1'))
put(99, 'BG_SKY12', band('1', '2'))
put(100, 'BG_SKY2', band('2'))
put(101, 'BG_SKY23', band('2', '3'))
put(102, 'BG_SKY3', band('3'))
# the surface row: sky above the waterline, ripples on top, water below
surf = []
for y in range(16):
    r = ''
    for x in range(16):
        if y == 0:
            r += 'C' if x % 8 < 5 else 'D'
        elif y == 1:
            r += 'D' if x % 8 < 3 else ('C' if x % 8 == 5 else '6')
        elif y == 2:
            r += 'C' if x % 8 == 6 else '6'
        else:
            r += '6' if y < 12 else ('7' if (x + y) % 2 == 0 else '6')
    surf.append(r)
put(103, 'BG_SURFACE', grid(surf))
seas = ['6', '7', '8', '9', 'A', 'B']
for i, c in enumerate(seas):
    put(104 + i * 2, 'BG_SEA%d' % i, band(c))
    if i + 1 < len(seas):
        put(105 + i * 2, 'BG_SEA%d%d' % (i, i + 1), band(c, seas[i + 1]))
# light rays under the surface
for i in range(2):
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            xx = x - (y // 3) - i * 8
            on = 2 <= xx % 16 <= 4 and (x + y) % 2 == 0
            r += 'E' if on else ('6' if i == 0 else '7')
        rows.append(r)
    put(115 + i, 'BG_RAY%d' % i, grid(rows))
# far icebergs on the horizon (rows of the sky)
berg_cap = [
    "3333333344333333",
    "3333333444433333",
    "3333334445543333",
    "3333344445554333",
    "3333444445555433",
    "3334444455555543",
    "3344444455555554",
    "3444444555555555",
    "4444445555555555",
    "4444445555555555",
    "4444455555555555",
    "4444455555555555",
    "4444555555555555",
    "4444555555555555",
    "4445555555555555",
    "4445555555555555",
]
put(117, 'BG_BERG_L', grid(berg_cap))
put(118, 'BG_BERG_R', grid([''.join('5' if c == '4' else ('3' if c == '3' else '5') for c in reversed(r)) for r in berg_cap]))
put(119, 'BG_BERG', band('5'))
put(120, 'BG_BERG_LIT', grid([('4' * (6 - (y // 4))).ljust(16, '5') for y in range(16)]))


# ---------------------------------------------------------------- sprites
ts.start_sprites(128)
spr, tall = ts.spr, ts.tall

# the pup, side view facing right, 16x16 on land and swimming level
PUP_SIT = [
    "", "", "",
    "..........kkk...",
    ".........kPPPkk.",
    "........kPPPPPPk",
    "........kPPPkPPk",
    "........kPPPkPPk",
    "...kkkkkkPPPPPPnk",
    "..kPPPPPPPPPPPkk",
    ".kPPPPPPPPPPPPk.",
    ".kPPPPPPPPPPPpk.",
    "kPkPPPPPPPPPppk.",
    "kPPkpppppppkpPk.",
    ".kk.kkkkkkk.kkk.",
]
PUP_SIT = [r[:16] for r in PUP_SIT]
spr('SPR_PUP_SIT', grid(PUP_SIT))
PUP_HUMP = [
    "", "",
    "..........kkk...",
    ".........kPPPkk.",
    "........kPPPPPPk",
    "........kPPPkPPk",
    "....kkk.kPPPkPPk",
    "...kPPPkkPPPPPPn",
    "..kPPPPPPPPPPPkk",
    ".kPPPPPPPPPPPPk.",
    ".kPPPPPPPPPPPpk.",
    "kPkPPPPPPPPPppk.",
    "kPPkppppppppPk..",
    ".kk.kkkkkkkkkk..",
]
spr('SPR_PUP_HUMP', grid(PUP_HUMP))
PUP_SLIDE = [
    "", "", "", "", "", "",
    "............kkk.",
    "...........kPPPk",
    "..kkkkkkkkkPPkPk",
    ".kPPPPPPPPPPPPPn",
    "kPPPPPPPPPPPPPkk",
    "kPPPPPPPPPPPPPk.",
    "kpPPPPPPPPPPPpk.",
    ".kpppppppppppk..",
    "..kkkkkkkkkkk...",
]
spr('SPR_PUP_SLIDE', grid(PUP_SLIDE))
PUP_SWIM1 = [
    "", "", "", "",
    "............kkk.",
    "..........kkPPPk",
    "..kkkkkkkkPPPkPk",
    ".kPPPPPPPPPPPPPn",
    "kPPPPPPPPPPPPPkk",
    "kPPPPPPPPPPPPk..",
    ".kppppppppkpk...",
    "..kkkkkkkk.kk...",
]
PUP_SWIM2 = [
    "", "", "", "",
    "............kkk.",
    "..........kkPPPk",
    "k.kkkkkkkkPPPkPk",
    "kkPPPPPPPPPPPPPn",
    ".kPPPPPPPPPPPPkk",
    "kkPPPPPPPPPPPk..",
    "k.kpppppppppk...",
    "...kkkkkkkkpk...",
    "............k...",
]
spr('SPR_PUP_SWIM1', grid(PUP_SWIM1))
spr('SPR_PUP_SWIM2', grid(PUP_SWIM2))
PUP_HURT = [
    "", "", "",
    "..........kkk...",
    ".........kPPPkk.",
    "........kPPPPPPk",
    "........kPPkPkPk",
    "........kPPPkPPk",
    "...kkkkkkPPkPkPn",
    "..kPPPPPPPPPPPkk",
    ".kPPPPPPPPPPPPk.",
    ".kPPPPPPPPPPPpk.",
    "kPkPPPPPPPPPppk.",
    "kPPkpppppppkpPk.",
    ".kk.kkkkkkk.kkk.",
]
spr('SPR_PUP_HURT', grid(PUP_HURT))
# vertical: head up, a native 16x32 sprite (flip Y for diving down)
PUP_UP1 = [
    "", "", "",
    "......kkkk......",
    ".....kPPPPk.....",
    "....kPPPPPPk....",
    "....kPkPPkPk....",
    "....kPkPPkPk....",
    "....kPPPPPPk....",
    "....kPPnnPPk....",
    "...kPPPPPPPPk...",
    "..kpkPPPPPPkpk..",
    ".kpk.kPPPPk.kpk.",
    ".kk..kPPPPk..kk.",
    ".....kPPPPk.....",
    "....kPPPPPPk....",
    "....kPPPPPPk....",
    "....kPPPPPPk....",
    "....kpPPPPpk....",
    ".....kPPPPk.....",
    ".....kpPPpk.....",
    "......kPPk......",
    "......kppk......",
    ".....kpkkpk.....",
    "....kpk..kpk....",
    "....kk....kk....",
]
PUP_UP2 = [
    "", "", "",
    "......kkkk......",
    ".....kPPPPk.....",
    "....kPPPPPPk....",
    "....kPkPPkPk....",
    "....kPkPPkPk....",
    "....kPPPPPPk....",
    "....kPPnnPPk....",
    "...kPPPPPPPPk...",
    "...kkPPPPPPkk...",
    "...kpkPPPPkpk...",
    "...kpkPPPPkpk...",
    "....kkPPPPkk....",
    "....kPPPPPPk....",
    "....kPPPPPPk....",
    "....kPPPPPPk....",
    "....kpPPPPpk....",
    ".....kPPPPk.....",
    ".....kpPPpk.....",
    "......kPPk......",
    "......kppk......",
    "......kpk.......",
    "......kpk.......",
    ".......kk.......",
]
tall('SPR_PUP_UP1', grid(PUP_UP1, 32))
tall('SPR_PUP_UP2', grid(PUP_UP2, 32))

# mother seal, sitting up on the shelf at the end of each shore (16x32)
MOM = [
    "", "", "", "", "",
    "......kkkk......",
    ".....kSSSSk.....",
    "....kSSSSSSk....",
    "....kSkSSSkSk...",
    "....kSSSSSSSnk..",
    "....kSSSSSSkk...",
    "....kSSSSSk.....",
    "...kSSSSSSSk....",
    "...kSSSTTSSk....",
    "..kSSSTTTTSSk...",
    "..kSStTTTTSSk...",
    "..kSSTTTTTTSk...",
    "..kSSTTTTTTSk...",
    ".kSSSTTTTTTSSk..",
    ".kStkTTTTTTktSk.",
    ".kSkkTTTTTTkkSk.",
    ".kk.kSTTTTSk.kk.",
    "....kSSSSSSSk...",
    "...kSStSSSSStk..",
    "..kSStkkkkkkSSk.",
    ".kStk......ktSk.",
    "kkkk........kkkk",
]
MOM = MOM + [""] * (32 - len(MOM))
MOM2 = [r for r in MOM]
MOM2[9] = "....kSSSSSSSSnk."
MOM2[10] = "....kSSSSkkkkk.."
tall('SPR_MOM1', grid(MOM, 32))
tall('SPR_MOM2', grid(MOM2, 32))

FISH1 = [
    "", "", "", "", "",
    "......kkkk......",
    "....kkFFFFkk..kk",
    "...kFwkFFFFFkkFk",
    "...kFFFFFFFFFFFk",
    "....kkeeeeekk.kk",
    "......kkkkk.....",
]
FISH2 = [
    "", "", "", "", "",
    "......kkkk....k.",
    "....kkFFFFkk.kFk",
    "...kFwkFFFFFkkFk",
    "...kFFFFFFFFFFk.",
    "....kkeeeeekk...",
    "......kkkkk.....",
]
spr('SPR_FISH1', grid(FISH1))
spr('SPR_FISH2', grid(FISH2))
CRAB1 = [
    "", "", "", "", "",
    "..kk........kk..",
    ".kKKk......kKKk.",
    ".kKk..k..k..kKk.",
    "..kKk.kwkwk.kKk.",
    "...kkkKKKKkkkk..",
    "....kKKKKKKk....",
    "...kKKKKKKKKk...",
    "...knnnnnnnnk...",
    "..kk.k.kk.k.kk..",
    ".k..k..k..k..k..",
]
CRAB2 = [
    "", "", "", "", "", "",
    "..kk........kk..",
    ".kKKk.k..k.kKKk.",
    ".kKk..kwkwk.kKk.",
    "..kkkkKKKKkkkk..",
    "....kKKKKKKk....",
    "...kKKKKKKKKk...",
    "...knnnnnnnnk...",
    "...k.k.kk.k.k...",
    "..k..k.k..k..k..",
]
spr('SPR_CRAB1', grid(CRAB1))
spr('SPR_CRAB2', grid(CRAB2))
GULL1 = [
    "", "",
    "kk..........kk..",
    "kmmk......kmmk..",
    ".kmmk....kmmk...",
    "..kmWk..kWmk....",
    "...kWWkkWWk.kk..",
    "....kWWWWWkkWWk.",
    ".....kWWWWWWWkak",
    "......kkWWWkkk..",
    "........kk......",
]
GULL2 = [
    "", "", "", "", "", "",
    "...........kk...",
    "....kkkkkkkWWk..",
    "..kkmmWWWWWWWkak",
    ".kmmmmkWWWWkkk..",
    "kmmmkk.kkkk.....",
    "kkkk............",
]
GULL_DIVE = [
    "", "", "",
    "......kk........",
    ".....kmmk.......",
    ".....kmWk.......",
    "....kmWWk.......",
    "....kmWWWk......",
    "....kmWWWk......",
    ".....kWWWk......",
    ".....kWWWk......",
    "......kWk.......",
    "......kak.......",
    ".......k........",
]
spr('SPR_GULL1', grid(GULL1))
spr('SPR_GULL2', grid(GULL2))
spr('SPR_GULL_DIVE', grid(GULL_DIVE))
JELLY1 = [
    "", "",
    ".....kkkkk......",
    "...kkJJJJJkk....",
    "..kJJwJJJJJJk...",
    "..kJJJJJJJJJk...",
    ".kJJJJJJJJJJJk..",
    ".kxxxxxxxxxxxk..",
    "..k.x.x.x.x.k...",
    "....x.x.x.x.....",
    "...x..x..x..x...",
    "...x.x...x.x....",
    "....x.x.x...x...",
    "...x...x.x.x....",
]
JELLY2 = [
    "", "", "",
    "....kkkkkk......",
    "..kkJJJJJJkk....",
    ".kJJwJJJJJJJk...",
    ".kJJJJJJJJJJk...",
    "kJJJJJJJJJJJJk..",
    "kxxxxxxxxxxxxk..",
    ".k.x.x.x.x.xk...",
    "...x.x.x.x.x....",
    "...x.x.x.x.x....",
    "....x.x.x.x.....",
    "....x.x.x.x.....",
]
spr('SPR_JELLY1', grid(JELLY1))
spr('SPR_JELLY2', grid(JELLY2))
# checkpoint buoy, 16x32: grey flag until the pup touches it
BUOY = [
    "", "", "",
    ".......k........",
    ".......kmmmk....",
    ".......kmmmmmk..",
    ".......kmmmmk...",
    ".......kmmk.....",
    ".......k........",
    ".......k........",
    ".......k........",
    ".....kkkkk......",
    "....kHHHHHk.....",
    "...kHHHHHHHk....",
    "...kwwwwwwwk....",
    "...kwwwwwwwk....",
    "...kHHHHHHHk....",
    "...kHHHHHHHk....",
    "...kwwwwwwwk....",
    "..kkkkkkkkkkk...",
    ".kzzzzzzzzzzzk..",
    "..kkkkkkkkkkk...",
]
BUOY = BUOY + [""] * (32 - len(BUOY))
tall('SPR_BUOY', grid(BUOY, 32))
tall('SPR_BUOY_ON', ts.recolor(grid(BUOY, 32), {'m': 'v'}))
SPLASH1 = [
    "", "", "", "", "", "", "",
    "......w...w.....",
    "...w..bw.wb..w..",
    "....b.bwwwb.b...",
    "..w.bbwbbbwbb.w.",
    "...bbwbbbbbwbb..",
]
SPLASH2 = [
    "", "", "",
    "...w.......w....",
    ".....w...w......",
    "..b....w....b...",
    ".....b...b......",
    "w..b.......b..w.",
    "..b..b...b..b...",
    ".b...........b..",
]
spr('SPR_SPLASH1', grid(SPLASH1))
spr('SPR_SPLASH2', grid(SPLASH2))
BUBBLE = ["", "", "", "", "",
          "......kk........",
          ".....kbbk.......",
          "....kbwbbk......",
          "....kbbbbk......",
          ".....kbbk.......",
          "......kk........"]
spr('SPR_BUBBLE', grid(BUBBLE))
STAR = ["", "", "",
        ".......k........",
        "......kek.......",
        "......kek.......",
        "..kkkkkeekkkk...",
        "...keeeweeek....",
        "....keeeeek.....",
        "....keekeek.....",
        "...keek.keek....",
        "...kkk...kkk...."]
spr('SPR_STAR', grid(STAR))
# HUD
HEART = ["", "", "",
         "..kkk...kkk.....",
         ".kHHHk.kHHHk....",
         "kHwHHHkHHHHHk...",
         "kHHHHHHHHHHHk...",
         "kHHHHHHHHHHHk...",
         ".kHHHHHHHHHk....",
         "..kHHHHHHHk.....",
         "...kHHHHHk......",
         "....kHHHk.......",
         ".....kHk........",
         "......k........."]
spr('SPR_HEART', grid(HEART))
spr('SPR_HEART_OFF', ts.recolor(grid(HEART), {'H': 'o', 'w': 'p'}))
AIR = ["", "", "", "",
       ".....kkkk.......",
       "....k6666k......",
       "...k6ww666k.....",
       "...k6w6666k.....",
       "...k666667k.....",
       "...k666777k.....",
       "....k6777k......",
       ".....kkkk......."]
AIR_OFF = ["", "", "", "", "",
           ".....5555.......",
           "....5....5......",
           "....5....5......",
           "....5....5......",
           "....5....5......",
           ".....5555.......",
           ""]
spr('SPR_AIR', grid(AIR))
spr('SPR_AIR_OFF', grid(AIR_OFF))
spr('SPR_FISH_ICON', grid(FISH1))


used = ts.save(ROOT)
print('tiles used:', used)

# Shared SNES art, owned locally so the game remains standalone.
from snes_glyphs import install as install_snes_glyphs
install_snes_glyphs(ROOT, used)
