#!/bin/bash
# Capturas do player no Glass UI, nos dois materiais. Nao entra na suite
# (*_shot.sh): janela GL.
#   NUVIO_SHOT_MOCK=<pasta do player-mockup> bash tests/player_glass_shot.sh <dir> [quadro...]
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
cc -DNV_SHOT_HOOKS "${sources[@]}" tests/player_glass_shot.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-player-glass-shot" \
  -O1 -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
D=$(mktemp -d); trap 'rm -rf "$D" "${TMPDIR:-/tmp}/nuvio-player-glass-shot"' EXIT
mkdir -p "$1"
for v in 0 1; do
  NUVIO_SHOT_VIDRO=$v NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" "${TMPDIR:-/tmp}/nuvio-player-glass-shot" "$@"
done
