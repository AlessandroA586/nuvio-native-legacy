#!/bin/bash
# Source guards for the episode-render review: highp faux bold, dithered
# merged veil, and holes only under fully opaque episode rows.
set -eu
cd "$(dirname "$0")/.."
python3 - <<'EOF'
import re, sys
g = open('src/gfx.c').read(); d = open('src/detail.c').read()
def block(s, start, end):
    i = s.index(start); return s[i:s.index(end, i)]
txt = block(g, '// GFX_TEXTO', '// GFX_FUNDO')
bold = block(txt, 'if (uPar.x < 0.0)', 'return;')
card = block(g, 'if (uVeu.x < 0.0) {', '// GFX_SOMBRA')
furos = block(d, 'static int epFuros(GfxRect *holes) {', '\n}\n')
fails = []
if not re.search(r'uniform NV_HP vec3 uPeso;', g): fails.append('uPeso not highp')
if re.search(r'(^|[^P] )vec2 (uv|p) =', bold) or 'uPar.y' in bold or 'uRaio' in bold:
    fails.append('faux-bold coords/offsets not highp')
if 'nv_dither(cor' not in card: fails.append('merged episode veil not dithered')
if 'gfx_opacidade_grupo' not in furos: fails.append('epFuros ignores row/group alpha')
for f in fails: print('FAIL:', f)
print('OK' if not fails else ''); sys.exit(1 if fails else 0)
EOF
