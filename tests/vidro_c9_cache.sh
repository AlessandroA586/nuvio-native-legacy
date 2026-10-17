#!/bin/bash
# Live-clock glass regression and same-run before/after PNGs, offline at 1080p.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=/Volumes/ExternalSSD/tmp/jobtmp
shots=/Volumes/ExternalSSD/tmp/rev/vidroc9-shots
mkdir -p "$shots"
export NUVIO_C9_CACHE=1 NUVIO_SHOT_VIDRO=1 NUVIO_SHOT_VIDRO_FOSCO=1
export NUVIO_SHOT_FUNDO=2 NUVIO_SHOT_VIDRO_QUAL=2
export OBJ="$TMPDIR/nuvio-vidroc9-cache-obj"
# macOS GL, with the webOS renderer branches enabled in gfx.c only.
mkdir -p "$OBJ"
gfx_source=src/gfx.c
if [ "${1:-after}" = before ]; then
  git show af7ab651f:src/gfx.c > "$OBJ/gfx-before.c"
  gfx_source="$OBJ/gfx-before.c"
fi
cc -O1 -g -DNV_WEBOS -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined -c "$gfx_source" -o "$OBJ/gfx.o"
# Preserve the specialized renderer object when the shared shot runner checks headers.
touch "$OBJ/gfx.o"
label=${1:-after}
mkdir -p "$shots/$label"
bash tests/detalhe_glass_shot.sh "$shots/$label" cache
