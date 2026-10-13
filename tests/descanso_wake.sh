#!/bin/bash
# Local reproduction with real GL and ASan/UBSan; isolated data, no TV needed.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-descanso-wake.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = src/main.c ] || sources+=("$source")
done
cc "${sources[@]}" tests/descanso_wake.c -Isrc -o "$work/test" \
  -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
# Same sdl2-compat/ASan loader workaround as fontesmortas_manual.sh.
[ ! -e /opt/homebrew/lib/libSDL3.dylib ] || ln -s /opt/homebrew/lib/libSDL3.dylib "$work/libSDL3.dylib"
NUVIO_DADOS="$work" "$work/test"
