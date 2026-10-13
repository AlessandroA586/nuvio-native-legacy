#!/bin/bash
# watch_state v2: desligar o addon apaga o evento pendente dele do diario.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ws-poda.XXXXXX")
trap 'rm -rf "$work"' EXIT
[ -n "${KEEP:-}" ] && trap - EXIT
# Hooks apenas nas copias temporarias: a troca real da lista termina antes da
# poda, e o envio espera essa publicacao entre resolver A e preparar o POST.
python3 - "$work" <<'PY'
from pathlib import Path
import sys
work = Path(sys.argv[1])
s = Path('src/addons.c').read_text()
# A poda mora em addons_definir_lista (1a ocorrencia), depois de listaMudou.
assert s.count('  wsenvio_podar();') == 2
s = 'void testeListaMudou(void);\n' + s.replace('  wsenvio_podar();', '  testeListaMudou();\n  wsenvio_podar();', 1)
(work / 'addons.c').write_text(s)
s = Path('src/watchstate_envio.c').read_text()
assert s.count('  if (d.viewers') + s.count('  if (s.decl.viewers') == 1
s = 'void testeTrocaSlot(void);\n' + s.replace('  if (d.viewers', '  testeTrocaSlot();\n  if (d.viewers').replace(
    '  if (s.decl.viewers', '  testeTrocaSlot();\n  if (s.decl.viewers')
(work / 'watchstate_envio.c').write_text(s)
PY
cc tests/watchstate_poda.c "$work/addons.c" src/watchstate.c "$work/watchstate_envio.c" src/js.c src/jsw.c \
  src/addonstats.c -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -std=gnu11 -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -Wno-deprecated-declarations \
  -ffunction-sections -fdata-sections -Wl,-dead_strip -lpthread -o "$work/t"
for caso in ${1:---desligar --remover --esquecer --antes-do-fio --trocar-slot --reordenar --manifesto-atrasado}; do
  rc=0
  "$work/t" "$caso" > "$work/saida" || rc=$?
  grep -v '^\[addons\]' "$work/saida" || true
  [ "$rc" -eq 0 ] || exit "$rc"
done
