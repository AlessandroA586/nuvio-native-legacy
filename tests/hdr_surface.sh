#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
binary=$(mktemp "${TMPDIR:-/tmp}/nuvio-hdr-surface.XXXXXX")
trap 'rm -f "$binary"' EXIT
cc -std=c99 -Wall -Wextra -Werror tests/hdr_surface.c -o "$binary"
"$binary"
# Existing opt-in defaults and device-local persistence regression.
bash tests/ajustes_padroes.sh
