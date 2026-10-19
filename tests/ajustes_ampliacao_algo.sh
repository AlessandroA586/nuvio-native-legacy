#!/bin/bash
# AJ_AMPLIACAO_ALGO: device-local upscaling algorithm setting.
# Verifies enum position, reading/writing, and Automatico maintains backward compatibility.
#   bash tests/ajustes_ampliacao_algo.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
TMPDIR="${TMPDIR:-/tmp}"
cc "${sources[@]}" tests/ajustes_ampliacao_algo.c -Isrc -o ${TMPDIR}/nuvio-ajustes-algo \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR}/nuvio-ajustes-algo
