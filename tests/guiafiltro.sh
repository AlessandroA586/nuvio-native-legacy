#!/bin/bash
# Pure rules and real Guide callbacks; each regression fails on the reviewed revision.
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
git show 5dddb0955:src/guia.c > "$work/guia.c"
git show 2e8eb26cd:src/guia.c > "$work/review.c"
cc "${flags[@]}" "${sources[@]}" "${common[@]}" \
  "-DGUIACACHE_SOURCE=\"$work/guia.c\"" -o "$work/baseline"
cc "${flags[@]}" "${sources[@]}" "${common[@]}" -o "$work/loader"
cc "${flags[@]}" "${sources[@]}" "${common[@]}" \
  "-DGUIACACHE_SOURCE=\"$work/review.c\"" -o "$work/review"
failed=0
for regression in pagination-all pagination-4k pagination-prefer visibility variants; do
  for version in baseline loader; do
    rc=0
    GUIAFILTRO_CASE="$regression" NUVIO_DADOS="$work/$version-$regression" \
      "$work/$version" "$work/$version-$regression" > "$work/$version-$regression.log" 2>&1 || rc=$?
    if [ "$version" = baseline ]; then
      case "$regression" in
        pagination-*) expected='Assertion failed: (pageRequests == 2)' ;;
        visibility) expected='Assertion failed: (nCanais == 1 && !strcmp(canais[0].id, "xtream:3"))' ;;
        variants) expected='Assertion failed: (!strcmp(v[0].nome, "X 4K")' ;;
      esac
      if [ "$rc" = 134 ] && rg -q -F "$expected" "$work/$version-$regression.log"; then
        printf 'PASS: reviewed baseline fails %s (rc=134)\n' "$regression"
        continue
      fi
    elif [ "$rc" = 0 ]; then
      printf 'PASS: %s regression\n' "$regression"
      continue
    fi
    printf 'FAIL: %s %s (rc=%s)\n' "$version" "$regression" "$rc"
    tail -n 3 "$work/$version-$regression.log"
    failed=1
  done
done
for regression in cap cache-refresh; do
  for version in review loader; do
    rc=0
    GUIAFILTRO_CASE="$regression" NUVIO_DADOS="$work/$version-$regression" \
      "$work/$version" "$work/$version-$regression" > "$work/$version-$regression.log" 2>&1 || rc=$?
    if [ "$version" = review ]; then
      if [ "$regression" = cap ]; then expected='Assertion failed: (pageRequests == 9 && sNCanais == 900)';
      else expected='Assertion failed: (restored == 4)'; fi
      if [ "$rc" = 134 ] && rg -q -F "$expected" "$work/$version-$regression.log"; then
        printf 'PASS: reviewed revision fails %s (rc=134)\n' "$regression"
        continue
      fi
    elif [ "$rc" = 0 ]; then
      printf 'PASS: %s regression\n' "$regression"
      continue
    fi
    printf 'FAIL: %s %s (rc=%s)\n' "$version" "$regression" "$rc"
    tail -n 3 "$work/$version-$regression.log"
    failed=1
  done
done
[ "$failed" = 0 ]
NUVIO_DADOS="$work/data" "$work/loader" "$work/data"
