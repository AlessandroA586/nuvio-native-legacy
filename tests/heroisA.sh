#!/bin/bash
# One native geometry regression and optional before/after PNGs; no network.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=/Volumes/ExternalSSD/tmp/jobtmp
export NUVIO_DADOS
NUVIO_DADOS=$(mktemp -d "$TMPDIR/nuvio-heroisA.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/home.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/heroisA.c -Isrc -o "$NUVIO_DADOS/test" -O1 -g \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations \
  >"$NUVIO_DADOS/build.log" 2>&1 || { tail -45 "$NUVIO_DADOS/build.log"; exit 1; }
set +e
"$NUVIO_DADOS/test" --fade
fade_rc=$?
"$NUVIO_DADOS/test" "$@"
layout_rc=$?
printf 'fade rc=%d; layout rc=%d\n' "$fade_rc" "$layout_rc"
[ "$fade_rc" -eq 0 ] && [ "$layout_rc" -eq 0 ]
