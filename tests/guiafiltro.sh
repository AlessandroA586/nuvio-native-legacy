#!/bin/bash
# Pure rules plus real Guide staging/callbacks. Baseline must fail the fixture.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-guiafiltro.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc src/guiafiltro.c tests/guiafiltro.c -Isrc -o "$work/rules" -O1 -g -Wall -Wextra
"$work/rules"
flags=(-ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-Wl,-dead_strip); else flags+=(-Wl,--gc-sections); fi
sources=(tests/guiafiltro_loader.c src/ltv.c src/js.c src/idcanal.c src/dados.c src/marco.c
  src/guiaaddons.c src/guiafiltro.c src/epg.c src/xtepg.c src/lembrete.c src/guialembrete.c
  src/fontecache.c src/cachearte.c src/cachedisco.c)
common=(-Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -O1 -g -Wno-macro-redefined)
cc "${flags[@]}" "${sources[@]}" "${common[@]}" -DGUIAFILTRO_BASELINE -o "$work/baseline"
rc=0
NUVIO_DADOS="$work/data-baseline" "$work/baseline" "$work/data-baseline" > "$work/baseline.log" 2>&1 || rc=$?
if [ "$rc" != 134 ] || ! rg -q 'sNCanais == .*modo' "$work/baseline.log"; then
  printf 'FAIL: unfiltered baseline must fail quality fixture (rc=%s)\n' "$rc"
  exit 1
fi
printf 'PASS: unfiltered baseline fails quality fixture (rc=134)\n'
cc "${flags[@]}" "${sources[@]}" "${common[@]}" -o "$work/loader"
NUVIO_DADOS="$work/data" "$work/loader" "$work/data"
