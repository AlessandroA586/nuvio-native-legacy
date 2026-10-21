#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc src/gpunivel.c src/perfiltv.c tests/gpunivel_escada.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-gpunivel-escada" \
  -DNV_GPUN_TESTE -DNV_WEBOS -O1 -g -Wall -Wextra -framework OpenGL -Wno-deprecated-declarations \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
"${TMPDIR:-/tmp}/nuvio-gpunivel-escada"
