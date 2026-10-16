#!/bin/bash
# Buffer size setting -> MiB and the free-RAM clamp. Ver o .c.
#   bash tests/buffer_tamanho.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-buffer.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -std=gnu11 -O1 -g -Wall -Wextra -Werror -Isrc src/cacheboost.c tests/buffer_tamanho.c -lpthread -lm -o "$work/t"
"$work/t"
