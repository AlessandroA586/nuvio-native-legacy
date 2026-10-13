#!/bin/bash
# Scrobble do Simkl a partir do player (issue #459). src/simkl.c e a maquina de
# decisao do Trakt (traktscrobble.c) contra uma rede falsa; token, nuvem e
# enfeite sao stubs. Sem SDL linkado, sem rede. A janela de espera e o espaco
# entre POSTs encolhem por -D para o teste caber em segundos.
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-simkl-scr-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc -O1 -g -Wall -Wextra -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  -DSMK_SCR_ESPACO_MS=0 -DSMK_SCR_ESPERA_MS=150 \
  src/simkl.c src/js.c src/traktscrobble.c tests/simkl_scrobble.c -o "$tmp/simkl_scrobble" -lpthread
"$tmp/simkl_scrobble"
