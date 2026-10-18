#!/bin/bash
# Action row gap after a focused button + "Assistir do começo" pill timing (pure).
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-acoes.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc -O1 -g -Wall -Wextra -Isrc tests/detail_acoes_gap.c -o "$dir/t" -lm
"$dir/t"
