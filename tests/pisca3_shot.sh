#!/bin/bash
# Apple TV layout + Fundo Frost + frosted glass: settled Home and title page
# frames must hash identical (tests/pisca3_shot.c). GL window: not in the suite.
#
#   bash tests/pisca3_shot.sh /Volumes/ExternalSSD/tmp/pisca3
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/nv-pisca3}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pisca3.XXXXXX")
export NUVIO_DADOS
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pisca3-build.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS" "$work"' EXIT
flags=(-O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
       -Wno-deprecated-declarations -Wno-macro-redefined -include tests/pisca3_gles.h ${NV_CFLAGS:-})
objects=()
for source in src/*.c src/dts/*.c tests/pisca3_shot.c; do
  [ "$source" = src/main.c ] && continue
  object="$work/$(basename "$source" .c).o"
  cc "${flags[@]}" -c "$source" -o "$object" &
  objects+=("$object")
done
wait
cc "${objects[@]}" -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -o "$work/shot"
"$work/shot" "$saida"
