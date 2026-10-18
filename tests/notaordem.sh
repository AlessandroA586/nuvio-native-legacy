#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Title-line order + Letterboxd survives a late load.
set -eu
cd "$(dirname "$0")/.."
cc tests/notaordem.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-notaordem -Wall -Wextra
${TMPDIR:-/tmp}/nuvio-notaordem
