#!/bin/bash
# FPS regression scenes, headless: percentiles, p95 gate, TV runner. Ver tests/fpscena.c.
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fpscena-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc -Wall -Wextra -O2 tests/fpscena.c src/fpscena.c -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib -lSDL2 -o "$tmp/t"
"$tmp/t"
