#!/bin/bash
# watch_state v2: desligar o addon apaga o evento pendente dele do diario.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ws-poda.XXXXXX")
trap 'rm -rf "$work"' EXIT
[ -n "${KEEP:-}" ] && trap - EXIT
cc tests/watchstate_poda.c src/addons.c src/watchstate.c src/watchstate_envio.c src/js.c src/jsw.c \
  src/addonstats.c -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -Wno-deprecated-declarations \
  -ffunction-sections -fdata-sections -Wl,-dead_strip -lpthread -o "$work/t"
for caso in --desligar --remover --esquecer --antes-do-fio; do
  "$work/t" "$caso" > "$work/saida"
  grep -v '^\[addons\]' "$work/saida" || true
done
