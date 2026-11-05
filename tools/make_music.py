#!/usr/bin/env python3
"""Writes the Tidepup music as ZMT (Zeal Music Tracker) files in assets/.

All tracks are original. Each pattern is 32 sixteenth steps on up to 3 voices; voice 4 is left
to the sound effects. A track holds at most 3 patterns (the game keeps 3 pattern buffers in the dead tileset blob).
Step tokens: '.' hold, '-' note off, 'C5' / 'F#4' notes, 'k s h' drum hits on the noise voice.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import zmt, voice  # noqa: E402

OUT = os.path.join(ROOT, 'assets')

# "Tidepool Stroll": the shore loop. F major, bouncy, 6 frames a step. A A' A A' C.
BEAT = "k . h . s . h . k . h k s . h . k . h . s . h . k . h k s . s ."
SHORE = [
    [   # A: F | C
        voice("square", 50, "F5 . A5 . C6 . . . A5 . G5 . F5 . . . E5 . G5 . C6 . . . G5 . E5 . C5 . . ."),
        voice("triangle", 100, "F2 . . . C3 . . . F2 . . . C3 . . . C2 . . . G2 . . . C2 . . . G2 . E2 ."),
        voice("noise", 25, BEAT),
        None,
    ],
    [   # A': Dm | Bb C
        voice("square", 50, "D5 . F5 . A5 . . . F5 . E5 . D5 . . . D5 . F5 . Bb5 . A5 . G5 . . . C6 . - ."),
        voice("triangle", 100, "D2 . . . A2 . . . D2 . . . A2 . . . Bb1 . . . F2 . . . C2 . . . G2 . . ."),
        voice("noise", 25, BEAT),
        None,
    ],
    [   # B: Gm | C7, back to the top
        voice("square", 50, "G5 . Bb5 . D6 . . . C6 . Bb5 . A5 . G5 . E5 . G5 . Bb5 . . . C6 . . . . . - ."),
        voice("triangle", 100, "G2 . . . D3 . . . G2 . . . D3 . . . C2 . . . G2 . . . C2 . E2 . G2 . . ."),
        voice("noise", 25, "k . h . s . h . k . h k s . h . k . k . s . k . s . s s s s s s"),
        None,
    ],
]

# Title: a slow lullaby over the waves. C major, 8 frames a step.
TITLE = [  # 2 patterns
    [
        voice("triangle", 100, "E5 . . . G5 . . . C6 . . . . . B5 . A5 . . . G5 . . . E5 . . . . . - ."),
        voice("square", 25, "C4 E4 G4 E4 C4 E4 G4 E4 A3 C4 E4 C4 A3 C4 E4 C4 F3 A3 C4 A3 F3 A3 C4 A3 G3 B3 D4 B3 G3 B3 D4 B3"),
        voice("noise", 0, "h . . . . . . . h . . . . . . . h . . . . . . . h . . . . . . ."),
        None,
    ],
    [
        voice("triangle", 100, "F5 . . . E5 . . . D5 . . . . . C5 . D5 . . . E5 . . . C5 . . . . . - ."),
        voice("square", 25, "F3 A3 C4 A3 F3 A3 C4 A3 G3 B3 D4 B3 G3 B3 D4 B3 A3 C4 E4 C4 A3 C4 E4 C4 C4 E4 G4 E4 C4 E4 G4 C5"),
        voice("noise", 0, "h . . . . . . . h . . . . . . . h . . . . . . . h . . . . . . ."),
        None,
    ],
]

# Jingles, one pattern each; the game stops a one-shot when its pattern wraps.
CLEAR = [[   # 5 frames a step
    voice("square", 50, "C5 . E5 . G5 . C6 . - . G5 . C6 . E6 . . . D6 . . . C6 . . . . . . . - ."),
    voice("triangle", 100, "C3 . . . G3 . . . C3 . . . E3 . . . F3 . . . G3 . . . C3 . . . - . . ."),
    voice("noise", 25, "k . h . s . h . k . k . s . s s k . . . s . . . k . . . . . . ."),
    None,
]]
OVER = [[   # 7 frames a step
    voice("square", 50, "G5 . . . F5 . . . Eb5 . . . D5 . . . C5 . . . . . . . . . . . - . . ."),
    voice("triangle", 100, "C3 . . . Bb2 . . . Ab2 . . . G2 . . . C2 . . . . . . . . . . . - . . ."),
    None,
    None,
]]

TRACKS = {
    "shore.zmt": zmt("TIDEPOOLS", 24, SHORE, [0, 1, 0, 1, 2]),
    "title.zmt": zmt("LULLABY", 32, TITLE, [0, 1]),
    "clear.zmt": zmt("HOME", 20, CLEAR, [0]),
    "over.zmt": zmt("LOST AT SEA", 28, OVER, [0]),
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, data in TRACKS.items():
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        print(f"{name}: {len(data)} bytes")
