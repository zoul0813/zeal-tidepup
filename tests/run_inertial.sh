#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
GAME="$HERE/.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
ZGDK="$ZGDK_PATH"
cc -std=gnu11 -w -g -fsanitize=address,undefined \
    '-D__sfr=static volatile unsigned char' '-D__at(x)=' \
    -I"$HERE/stub" -I"$GAME/src" -I"$ZGDK/include" \
    -I"$ZOS_PATH/sdk/sdcc/include" -I"$ZVB_SDK_PATH/include" \
    "$HERE/host_inertial.c" "$HERE/physics_reference.c" \
    "$ZGDK/src/physics/body/"*.c "$ZGDK/src/utils/clamp_i16.c" "$ZGDK/src/physics/inertial/"*.c \
    "$ZGDK/src/tilemap/flags/"*.c -o "$OUT/inertial"
"$OUT/inertial"
