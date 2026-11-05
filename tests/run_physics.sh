#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
GAME="$HERE/.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
cc -std=gnu11 -w -g -fsanitize=address,undefined \
    '-D__sfr=static volatile unsigned char' '-D__at(x)=' \
    -I"$HERE/stub" -I"$GAME/src" \
    -I"$ZGDK_PATH/include" -I"$ZOS_PATH/sdk/sdcc/include" -I"$ZVB_SDK_PATH/include" \
    "$HERE/host_water.c" "$ZGDK_PATH/src/physics/body"/*.c "$ZGDK_PATH/src/utils/clamp_i16.c" "$ZGDK_PATH/src/tilemap/flags"/*.c "$GAME/src/water.c" -o "$OUT/physics"
"$OUT/physics"
