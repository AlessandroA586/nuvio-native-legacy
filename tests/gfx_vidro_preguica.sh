#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Requires a native GL window; compares framebuffer pixels without TV or network.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-gfx-vidro-preguica.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ] && [ "$source" != src/gfx.c ]; then sources+=("$source"); fi
done
build() {
cc "${sources[@]}" tests/gfx_vidro_preguica.c -Isrc -o "$tmp/teste" "$@" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
}
rc=0
prefix="${NV_GLASS_SHOT_PREFIX:-/Volumes/ExternalSSD/tmp/rev/tpk60preta/after}"
mkdir -p "$(dirname "$prefix")"
build
for mode in startup failure normal; do
  NV_GLASS_SHOT_PREFIX="$prefix-$mode" "$tmp/teste" "$mode" || rc=1
done
build -DNV_TEST_TPK
NV_GLASS_SHOT_PREFIX="$prefix-tpk" "$tmp/teste" startup || rc=1
exit "$rc"
