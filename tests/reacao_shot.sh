#!/bin/bash
# Capturas do cartao "O que achou?" (reacao.c), em BMP. Nao entra na suite.
#
#   bash tests/reacao_shot.sh /pasta/de/saida
set -eu
cd "$(dirname "$0")/.."
SAIDA=${1:-/tmp/nuvio-reacao-shot}
mkdir -p "$SAIDA"
NUVIO_DADOS=$(mktemp -d /tmp/nuvio-reacao-shot-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS" /tmp/nuvio-reacao-shot-bin /tmp/nuvio-reacao-shot-bin.dSYM' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = src/main.c ] && continue
  sources+=("$source")
done
cc "${sources[@]}" tests/reacao_shot.c -Isrc -o /tmp/nuvio-reacao-shot-bin \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-reacao-shot-bin "$SAIDA"
