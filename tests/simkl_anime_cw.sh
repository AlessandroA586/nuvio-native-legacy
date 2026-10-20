#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-simkl-anime-XXXXXX")
trap 'rm -rf "$task_tmp"' EXIT
cc -O1 -g -Wall -Wextra -ffunction-sections -fdata-sections -Wl,-dead_strip -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  src/simkl.c src/trakt.c src/js.c src/traktscrobble.c src/metaep.c tests/simkl_anime_cw.c \
  -o "$task_tmp/test" -lpthread
"$task_tmp/test"
