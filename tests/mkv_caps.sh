#!/bin/bash
# src/redeurl.c anda junto com src/rede.c: rede.c imprime URL redigida no log e
# a redacao mora naquele arquivo (ver o cabecalho dele). Quem compila por glob
# de src/*.c src/dts/*.c nao precisa pensar nisso; quem lista fontes a mao, sim.
set -eu
cd "$(dirname "$0")/.."
. tests/mkvcore_flag.sh
cc -Isrc tests/mkv_caps.c src/rede.c src/redeurl.c -o /tmp/nuvio-mkv-caps-tests \
  -I/opt/homebrew/include -Wno-deprecated-declarations ${MKVCORE_CC[@]+"${MKVCORE_CC[@]}"} 2>/dev/null || \
cc -Isrc tests/mkv_caps.c src/rede.c src/redeurl.c -o /tmp/nuvio-mkv-caps-tests -I/opt/homebrew/include ${MKVCORE_CC[@]+"${MKVCORE_CC[@]}"}
/tmp/nuvio-mkv-caps-tests
