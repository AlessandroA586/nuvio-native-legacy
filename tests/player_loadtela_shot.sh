#!/bin/bash
# A3 loading screen captures (3 moments). Out of the suite (*_shot.sh): GL window.
#   bash tests/player_loadtela_shot.sh [outdir] [src-dir]
# Default: depois/ from this tree, antes/ from ANTES_REF's src (git archive;
# c0dc799a3 = the tree right before the A3 screen).
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=${TMPDIR:-/Volumes/ExternalSSD/tmp/jobtmp}
mkdir -p "$TMPDIR"
work=$(mktemp -d "$TMPDIR/nuvio-loadtela-shot.XXXXXX")
trap 'rm -rf "$work"' EXIT
shot() {   # $1 = src dir, $2 = outdir
  local sources=() source
  for source in "$1"/*.c "$1"/dts/*.c; do [ "$(basename "$source")" != main.c ] && sources+=("$source"); done
  cc -DNV_SHOT_HOOKS "${sources[@]}" tests/player_loadtela_shot.c -I"$1" -o "$work/shot" \
    -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
    -Wl,-U,_player_morph_de -Wno-deprecated-declarations -Wno-macro-redefined
  mkdir -p "$2" "$work/dados"
  NUVIO_DADOS="$work/dados" NUVIO_TESTE_DIR="$work/dados" "$work/shot" "$2"
}
if [ $# -ge 1 ]; then shot "${2:-src}" "$1"; exit; fi
out=/Volumes/ExternalSSD/tmp/loadtela
mkdir -p "$work/antes"
git archive "${ANTES_REF:-c0dc799a3}" src | tar -x -C "$work/antes"
shot "$work/antes/src" "$out/antes"
shot src "$out/depois"
