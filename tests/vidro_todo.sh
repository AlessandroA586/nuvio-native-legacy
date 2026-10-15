#!/bin/bash
# Vidro em tudo: the shared materials follow "Opacidade do vidro" and the panels
# share one tone (tests/vidro_todo.c), and every inventoried surface calls one
# of them (grep below; table in docs/issues/vidro-todo.md).
#   bash tests/vidro_todo.sh            (RAIZ=<other checkout> runs it on that tree)
set -u
cd "$(dirname "$0")/.."
t=$PWD/tests
raiz=${RAIZ:-$PWD}
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-vidro-todo.XXXXXX")
trap 'rm -rf "$work"' EXIT
fail=0

# 1) Every inventoried surface draws through the shared material (file|regex).
need='
src/ilha.c|gfx_vidro_material\(R, raio, fundo, A\)
src/menu.c|gfx_vidro_material\(r, raio, al, a\)
src/menu.c|gfx_vidro_material\(R, raioPx / R.h, TV_MIOLO_MIN, A\)
src/recenviar.c|^  plrui_material\(p, RE_RAIO, 1, a\);
src/homevazio.c|gfx_vidro_material\(p, 0.06f, GFX_VIDRO_DENS, alfa\)
src/vertudo.c|gfx_vidro_material\(\(GfxRect\)\{px, py, pw, ph\}
src/plrui.c|if \(ajustes_vidro\(\)\) gfx_vidro_painel\(r, 0.5f, 0.55f, a\)
src/estatvivoui.c|if \(ajustes_vidro\(\)\) gfx_vidro_painel\(r, 18.0f / r.h
src/amigoperfil.c|if \(ajustes_vidro\(\)\) gfx_vidro_painel\(r, 18.0f / r.h
'
while IFS='|' read -r f re re2; do
  [ -z "$f" ] && continue
  grep -qE "$re${re2:+|$re2}" "$raiz/$f" || { echo "FAIL $f: missing $re"; fail=1; }
done <<< "$need"
# The two private panel tones are gone.
grep -nE "GFX_VIDRO_PAINEL, (0\.09f|15\.0f / 255\.0f)" "$raiz"/src/*.c && { echo "FAIL private glass tone"; fail=1; }

# 2) Runtime: opacity and tone (GL, hidden window).
sources=()
for source in "$raiz"/src/*.c "$raiz"/src/dts/*.c; do
  [ "$source" != "$raiz/src/main.c" ] && sources+=("$source")
done
if cc "${sources[@]}" "$t/vidro_todo.c" -I"$raiz/src" -o "$work/test" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined >"$work/cc.log" 2>&1; then
  mkdir -p "$work/dados"
  (cd "$raiz" && NUVIO_DADOS="$work/dados" NUVIO_TESTE_DIR="$work/dados" "$work/test") || fail=1
else
  cat "$work/cc.log" >&2; fail=1
fi

[ "$fail" -eq 0 ] && echo "vidro todo: PASS" || echo "vidro todo: FAIL"
exit $fail
