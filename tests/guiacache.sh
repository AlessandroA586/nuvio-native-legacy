#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Cache por fonte do guia: fonte removida nao volta do disco; a que so falhou
# a rede fica. Ver tests/guiacache.c.
#
#   bash tests/guiacache.sh
#   SANITIZE=1 bash tests/guiacache.sh
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-guiacache.XXXXXX)
trap 'rm -rf "$work"' EXIT
flags=(-ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-Wl,-dead_strip); else flags+=(-Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
if [ "${GUIACACHE_BASELINE:-0}" = 1 ]; then
  git show ba0639cf0:src/guia.c > "$work/guia.c"
  flags+=(-DGUIACACHE_BASELINE "-DGUIACACHE_SOURCE=\"$work/guia.c\"")
fi
cc ${flags[@]+"${flags[@]}"} tests/guiacache.c \
  src/ltv.c src/js.c src/idcanal.c src/dados.c src/marco.c src/guiaaddons.c \
  src/epg.c src/xtepg.c src/lembrete.c src/guialembrete.c \
  src/fontecache.c src/cachearte.c src/cachedisco.c \
  -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -O1 -g -Wno-macro-redefined -o "$work/test"
NUVIO_DADOS="$work/dados" "$work/test" "$work/dados"
