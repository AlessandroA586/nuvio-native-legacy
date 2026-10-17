#!/bin/bash
# Text scale keeps the Home anchors in place (#205). Hard 48 s budget.
set -euo pipefail
cd "$(dirname "$0")/.."
w=$(mktemp -d "${TMPDIR:-/tmp}/nv-ancoras.XXXXXX")
trap 'rm -rf "$w"' EXIT
flags=(-O0 -w -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
srcs=()
for s in src/*.c src/dts/*.c tests/escala_ancoras.c; do
  case "$s" in src/main.c|src/home.c) continue;; esac
  srcs+=("$s")
done
printf '%s\n' "${srcs[@]}" | timeout 40 xargs -P 12 -I{} sh -c \
  'cc "$@" -c "$0" -o "'"$w"'/$(echo "$0" | tr / _).o"' {} "${flags[@]}"
cc "$w"/*.o -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -o "$w/t"
NUVIO_DADOS="$w" timeout 5 "$w/t"
