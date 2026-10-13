#!/bin/bash
# watch_state v2 push, passo 3 (fio de entrega com rede/disco/addons falsos).
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ws-envio.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/watchstate_envio.c src/watchstate_envio.c src/watchstate.c src/js.c src/jsw.c -Isrc -std=gnu11 -Wall -Wextra \
  -Wno-unused-function -Wno-macro-redefined -lpthread -o "$work/t"
"$work/t"
