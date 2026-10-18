#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/issue607-subtitle.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/issue607_subtitle.c tests/addons_legendas_prazo.c src/linguas.c src/js.c src/legextras.c \
  -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -std=gnu11 -pthread -ffunction-sections -fdata-sections -Wl,-dead_strip \
  -Wno-macro-redefined -Wno-return-type -o "$work/test"
"$work/test"
