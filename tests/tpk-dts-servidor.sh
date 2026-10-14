#!/bin/bash
# On the host: the .tpk DTS HLS server (real src/dts/dts_hls_tpk.c) with a fake
# converter. See the top of tests/tpk-dts-servidor.c.
set -eu
cd "$(dirname "$0")/.."
OUT="${TMPDIR:-/tmp}/nuvio-tpk-dts-servidor"
cc -O1 -g -Wall -DNV_TPK -Isrc/dts tests/tpk-dts-servidor.c src/dts/dts_hls_tpk.c -lpthread -lm -o "$OUT"
"$OUT"
