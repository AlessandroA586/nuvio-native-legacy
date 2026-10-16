#!/bin/bash
# Frost over live video: every player scene over a full 16:9 hole must draw
# glass with a blur source (the baked title art), not flat. Mac GL window.
#   bash tests/player_fosco.sh [capture-dir]
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fosco-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
sources=()
for s in src/*.c src/dts/*.c; do
  [ "$s" = src/main.c ] || sources+=("$s")
done
cc -DNV_SHOT_HOOKS "${sources[@]}" tests/player_fosco.c -Isrc -Itests -o "$tmp/t" \
  -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
caps="${1:-$tmp/caps}"
mkdir -p "$tmp/dados" "$caps"
out=$(NUVIO_SHOT_VIDRO=1 NUVIO_SHOT_VIDRO_FOSCO=1 NUVIO_DADOS="$tmp/dados" NUVIO_TESTE_DIR="$tmp/dados" \
  "$tmp/t" "$caps" fosco-osd fosco-audio fosco-stats 2>/dev/null | grep '^F ')
echo "$out"
[ "$(echo "$out" | wc -l)" -eq 3 ] || { echo "FAIL: missing scenes"; exit 1; }
if echo "$out" | grep -q 'fonte=0$'; then echo "FAIL: player glass over video is flat"; exit 1; fi
echo PASS
