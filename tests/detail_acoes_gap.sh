#!/bin/bash
# Action row gap after/before a focused button (pure) + guard that heroWeb uses the helpers.
set -euo pipefail
cd "$(dirname "$0")/.."
src=${DETAIL_C:-src/detail.c}
[ "$(grep -c 'det_avanco(' "$src")" -ge 2 ] || { echo "FALHOU: heroWeb must advance via det_avanco (play + restart)"; exit 1; }
[ "$(grep -c 'det_foco_extra(NV_DETW2_CIRC' "$src")" -ge 2 ] || { echo "FALHOU: grouped and ungrouped circles must use det_foco_extra"; exit 1; }
! grep -q 'NV_DET_INICIO' "$src" || { echo "FALHOU: restart pill timing must stay 160/120"; exit 1; }
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-acoes.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc -O1 -g -Wall -Wextra -Isrc tests/detail_acoes_gap.c -o "$dir/t" -lm
"$dir/t"
