#!/bin/bash
# Parser de links externos (tests/deeplink.c).
#
#   bash tests/deeplink.sh
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/nv-deeplink-tests"
cc ${SANITIZE:+-fsanitize=address,undefined} src/deeplink.c tests/deeplink.c -Isrc -o "$out" -O1 -g -Wall
"$out"
