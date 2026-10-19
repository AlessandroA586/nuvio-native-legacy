#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Layout Imersiva (experimental): enum, fantasmas, troca interrompida, abrir
# no lugar. Ver tests/imersiva.c.
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra -O2 tests/imersiva.c src/imersiva.c -Isrc -I/opt/homebrew/include/SDL2 -I/opt/homebrew/include -o "$TMPDIR/nuvio-imersiva-test" -lm
"$TMPDIR/nuvio-imersiva-test"
