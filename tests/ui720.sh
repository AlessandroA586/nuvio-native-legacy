#!/bin/bash
# Offline Home/Settings PNGs, sharpness regression, alpha equality and GPU timer.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ui720.XXXXXX")
trap 'rm -rf "$work"' EXIT
out="${1:-/Volumes/ExternalSSD/tmp/rev/ui720}"
mkdir -p "$out" "$work/data"
printf 'animacoes 0\nidioma 1\nhomeLayoutLocal 0\n' > "$work/data/ajustes.txt"
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" != src/main.c ] && sources+=("$source")
done
cc "${sources[@]}" tests/ui720.c -Isrc -o "$work/test" -O1 \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$work/data" "$work/test" "$out" > "$out/results.txt" 2>&1 || {
  tail -20 "$out/results.txt"
  exit 1
}
tail -16 "$out/results.txt"
