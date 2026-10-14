#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# A ordem dos itens da fileira de colecao (2.0.5): as quatro ordens sobre as
# pastas de um grupo e a escolha gravada por fileira. Sem SDL, sem rede.
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} -DFIL_TESTE src/colecoes.c src/redeurl.c src/js.c src/fileiras.c \
  tests/colordem.c -Isrc -o "${TMPDIR}/nuvio-colordem-tests" -O1 -g -Wall -Wextra \
  -Wno-deprecated-declarations -Wno-macro-redefined
"${TMPDIR}/nuvio-colordem-tests"
