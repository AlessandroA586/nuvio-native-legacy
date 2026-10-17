#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ilhas-fix.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" != src/main.c ] && sources+=("$source")
done
cc "${sources[@]}" tests/vidro_ilhas_fix.c -Isrc -o "$work/test" -O1 \
  -DNV_REC_URL='"http://127.0.0.1:1"' \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p "$work/dados" /Volumes/ExternalSSD/tmp/vidroilhas/fix
NUVIO_DADOS="$work/dados" NUVIO_TESTE_DIR="$work/dados" \
NUVIO_FIX_PNGS=/Volumes/ExternalSSD/tmp/vidroilhas/fix \
NUVIO_FIX_PHASE="${NUVIO_FIX_PHASE:-after}" "$work/test"
