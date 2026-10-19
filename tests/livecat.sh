#!/bin/bash
# Live TV category suggestions (src/livecat.c): section per unclassified channel,
# disk cache per base name, batches of 200, no network for classified channels.
# Includes Worker HTTP contract, shared limiter and cleanup regressions.
#
#   bash tests/livecat.sh
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-livecat-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc src/livecat.c src/dados.c src/js.c tests/livecat.c -Isrc -DNV_REC_URL='"http://rec.test"' \
  -o "$tmp/unidade" -O1 -g -Wall -Wextra -Wno-unused-function
NUVIO_DADOS="$tmp/unit" "$tmp/unidade" | grep -v "^\[dados\]"
NUVIO_DADOS="$tmp/unit" "$tmp/unidade" restart | grep -v "^\[dados\]"
flags=(-ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-Wl,-dead_strip); else flags+=(-Wl,--gc-sections); fi
cc tests/livecat_guia.c src/livecat.c src/dados.c src/ltv.c src/js.c src/idcanal.c -Isrc \
  -DNV_REC_URL='"http://rec.test"' -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -O1 -g -Wno-macro-redefined "${flags[@]}" -o "$tmp/guia"
NUVIO_DADOS="$tmp/guide" "$tmp/guia"
node servidor/recomendacoes/teste-livetvcat.mjs
