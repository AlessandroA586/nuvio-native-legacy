#!/bin/bash
# Enquete na ilha (N3): cliente contra um servidor de mentira, sem rede nem janela.
#   bash tests/enquete.sh
# Compila tudo MENOS src/enquete.c, que o teste inclui. Escreve so em NUVIO_DADOS
# (pasta temporaria; o teste se recusa a rodar fora dela).
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d /tmp/nuvio-enq-dados.XXXXXX)
export NUVIO_DADOS
binary="$(mktemp /tmp/nuvio-enquete.XXXXXX)"
trap 'rm -rf "$NUVIO_DADOS" "$binary"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/enquete.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/enquete.c -Isrc -o "$binary" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -w
"$binary"
