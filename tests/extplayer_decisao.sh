#!/bin/bash
# Player setting (Internal / External / Ask) + headers -> decision. Ver o .c.
#   bash tests/extplayer_decisao.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-extplayer.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -std=gnu11 -O1 -g -Wall -Wextra -Werror -Isrc src/extplayer.c tests/extplayer_decisao.c -o "$work/t"
"$work/t"
