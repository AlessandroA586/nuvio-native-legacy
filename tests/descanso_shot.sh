#!/bin/bash
# Capturas da tela de descanso (vitrine e relogio). Precisa de janela GL; nao
# entra na suite. Ver tests/descanso_shot.c.
#   bash tests/descanso_shot.sh [prefixo]
set -eu
cd "$(dirname "$0")/.."
fontes=()
for f in src/*.c src/dts/*.c; do [ "$f" != src/main.c ] && fontes+=("$f"); done
cc "${fontes[@]}" tests/descanso_shot.c -Isrc -o /tmp/nuvio-descanso-shot \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
D=$(mktemp -d); trap 'rm -rf "$D"' EXIT
NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" NUVIO_SHOT_FONTE="${NUVIO_SHOT_FONTE:-3}" \
  /tmp/nuvio-descanso-shot "${1:-/tmp/nuvio-descanso}"
