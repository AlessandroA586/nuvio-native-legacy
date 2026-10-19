#!/bin/bash
# Offline GL regression and review captures; no network or device required.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
export NUVIO_DADOS
NUVIO_DADOS=$(mktemp -d "$TMPDIR/nuvio-epinfo.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS"' EXIT
out=/Volumes/ExternalSSD/tmp/rev/epinfo
mkdir -p "$out"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/detail_epinfo_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
python3 tests/detail_epinfo.py
"$NUVIO_DADOS/shot" "$out" "${1:-depois}"
