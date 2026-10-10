#!/bin/bash
# The .tpk source crop, with no TV. See tests/tpk-crop.c.
set -eu
cd "$(dirname "$0")/.."
SDL="$(sdl2-config --cflags) $(sdl2-config --libs)"
# -DNV_TPK40 because the source crop is the 4/5 path (see video_janela_fonte:
# the 6+/9 builds keep today's behaviour until one can be tested).
cc -O1 -g -Wall -DNV_TPK -DNV_TPK40 -Isrc tests/tpk-crop.c src/video_tpk.c src/velocidade.c \
  src/faixasmkv.c src/audioinfo.c src/capmkv.c src/mkv.c tests/capmkv_stub.c -lpthread $SDL \
  -o "${TMPDIR:-/tmp}/nuvio-tpk-crop"
"${TMPDIR:-/tmp}/nuvio-tpk-crop"
