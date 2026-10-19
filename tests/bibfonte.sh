#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-bibfonte.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
root="$PWD"
flags=()
if [ -n "${BIBFONTE_REV:-}" ]; then
  mkdir -p "$tmp/tree/tests"
  git archive "$BIBFONTE_REV" src | tar -x -C "$tmp/tree"
  cp tests/bibfonte.c tests/bibfonte_library.c tests/bibfonte_discovery.c tests/bibfonte_menu.c tests/bibfonte_detail.c tests/bibfonte_shots.inc tests/nvtmp.h "$tmp/tree/tests/"
  root="$tmp/tree"
  if ! rg -q listaFontes "$root/src/catalogo.h"; then flags+=(-DBIBFONTE_BASELINE); fi
fi
sources=()
for source in "$root"/src/*.c "$root"/src/dts/*.c; do
  case "${source##*/}" in main.c|ajustes.c|ilhasalvar.c|biblioteca.c|descoberta.c|ctxmenu.c|detail.c) continue;; esac
  sources+=("$source")
done
cc "${flags[@]+${flags[@]}}" "${sources[@]}" "$root/tests/bibfonte.c" "$root/tests/bibfonte_library.c" "$root/tests/bibfonte_discovery.c" "$root/tests/bibfonte_menu.c" "$root/tests/bibfonte_detail.c" \
  -I"$root/src" -O0 -ffunction-sections -fdata-sections -Wl,-dead_strip -o "$tmp/test" \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/test" "$@"
if [ "$#" -gt 0 ]; then "$tmp/test"; fi
