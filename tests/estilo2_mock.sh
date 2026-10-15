#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Mock das tres direcoes do seletor de estilo/ordem da fileira (job
# 205-estilo2): 9 PNGs 1920x1080 {A,B,C}-{redonda,tamanho,ordem}.png sobre a
# Home real. Nao entra na suite: precisa de janela GL e de olho humano.
#
#   bash tests/estilo2_mock.sh [pasta]   (padrao /Volumes/ExternalSSD/tmp/rev/estilo2)
set -eu
cd "$(dirname "$0")/.."
saida="${1:-/Volumes/ExternalSSD/tmp/rev/estilo2}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "$TMPDIR/nuvio-estilo2-dados.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
# Sem rede: a Home pede meta das fileiras; a libcurl segue o proxy do ambiente
# e um proxy morto recusa na hora.
export http_proxy=http://127.0.0.1:9 https_proxy=http://127.0.0.1:9 \
  HTTPS_PROXY=http://127.0.0.1:9 ALL_PROXY=http://127.0.0.1:9 all_proxy=http://127.0.0.1:9
unset no_proxy NO_PROXY
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
# Nunca executar um binario velho se o compilador falhar.
cc -DAJUSTES_TESTE "${sources[@]}" tests/estilo2_mock.c -Isrc -o "$NUVIO_DADOS/mock" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wall -Wno-deprecated-declarations -Wno-macro-redefined
"$NUVIO_DADOS/mock" "$saida"
for f in "$saida"/*.bmp; do
  [ -e "$f" ] || continue
  sips -s format png "$f" --out "${f%.bmp}.png" >/dev/null 2>&1 && rm -f "$f"
done
