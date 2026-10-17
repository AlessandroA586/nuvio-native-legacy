#!/bin/bash
# Real Home/Search/Detail/Settings/clock draws and PNGs at 80/85/100/115%.
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/Volumes/ExternalSSD/tmp/jobtmp}"
mkdir -p "$TMPDIR"
w=$(mktemp -d "$TMPDIR/nv-ancoras.XXXXXX")
trap 'rm -rf "$w"' EXIT
export NV_ANCHOR_SHOTS="${NV_ANCHOR_SHOTS:-/Volumes/ExternalSSD/tmp/rev/escalafixa-shots}"
mkdir -p "$NV_ANCHOR_SHOTS"
# Optional independent failure proof against a committed production revision.
if [ -n "${NV_ANCHOR_BASELINE:-}" ]; then
  mkdir -p "$w/base" "$w/fixtures"
  for source in home.c busca.c detail.c ajustes.c ajustes_ux_desenho.inc ajustes_ux_v2.inc; do
    git show "$NV_ANCHOR_BASELINE:src/$source" > "$w/base/$source"
  done
  python3 - "$w" <<'PY'
import pathlib, sys
work=pathlib.Path(sys.argv[1])
for source in pathlib.Path('tests').glob('escala_ancoras*.c'):
    text=source.read_text().replace('../src/',str(work/'base')+'/')
    (work/'fixtures'/source.name).write_text(text)
PY
fi
flags=(-DAJUSTES_TESTE -O0 -w -Isrc -Itests -I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
# Instrument the renderer itself, including internal gfx_cor/material calls.
python3 - "$w/gfx.c" <<'PY'
import pathlib, sys
source=pathlib.Path('src/gfx.c').read_text()
needle='  GFX_TR_RECT(r.x, r.y, r.w, r.h);'
assert needle in source
source=source.replace(needle, '  anchor_probe(r, tex, modo, foco, parx, pary, raio, cr, cg, cb, ca);\n'+needle,1)
pathlib.Path(sys.argv[1]).write_text('#include "gfx.h"\nvoid anchor_probe(GfxRect, GLuint, GfxModo, float, float, float, float, float, float, float, float);\n'+source)
PY
srcs=("$w/gfx.c")
for s in src/*.c src/dts/*.c tests/escala_ancoras*.c; do
  case "$s" in src/main.c|src/gfx.c|src/home.c|src/busca.c|src/detail.c|src/ajustes.c) continue;; esac
  srcs+=("$s")
done
if [ -n "${NV_ANCHOR_BASELINE:-}" ]; then
  filtered=()
  for source in "${srcs[@]}"; do
    case "$source" in tests/escala_ancoras*.c) filtered+=("$w/fixtures/${source##*/}");; *) filtered+=("$source");; esac
  done
  srcs=("${filtered[@]}")
fi
printf '%s\n' "${srcs[@]}" | xargs -P 12 -I{} sh -c \
  'cc "$@" -c "$0" -o "'"$w"'/$(echo "$0" | tr / _).o"' {} "${flags[@]}"
cc "$w"/*.o -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -o "$w/t"
NUVIO_DADOS="$w" "$w/t"
