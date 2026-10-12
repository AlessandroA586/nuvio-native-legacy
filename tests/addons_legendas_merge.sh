#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-subtitles-merge.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/addons_legendas_merge.c tests/addons_legendas_prazo.c src/linguas.c src/js.c src/legextras.c -Isrc \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -std=gnu11 -pthread -ffunction-sections -fdata-sections -Wl,-dead_strip -Wall -Wextra \
  -Wno-unused-function -Wno-macro-redefined -Wno-missing-field-initializers -o "$work/test"
"$work/test" > "$work/log"
rg -F '[legendas] addon avisou: [❌] OpenSubtitles V3+ - 500 - Internal Server Error' "$work/log"
rg -F '[legendas] addon avisou: upstream failed' "$work/log"
rg '^PASS:' "$work/log"
