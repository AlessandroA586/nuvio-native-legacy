#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
if [ "$(uname)" = Darwin ]; then gc=-Wl,-dead_strip; else gc=-Wl,--gc-sections; fi
cc tests/legenda_dobrada.c -Isrc "${sdl_flags[@]}" \
  -std=gnu11 -O1 -g -ffunction-sections -fdata-sections "$gc" \
  -Wall -Wextra -Wno-unused-function -o "$work/test"
"$work/test"
# The ownership check gates only the primary, leaving the secondary independent.
python3 - <<'PY'
from pathlib import Path
s = Path('src/player.c').read_text()
body = s.split('static void desenharLegendaPrincipal(float *topoPilha){', 1)[1]
assert body.lstrip().startswith('if (!faixas_legenda_overlay()) return;')
PY
