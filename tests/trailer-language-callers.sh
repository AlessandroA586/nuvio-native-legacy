#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
TR_DIR=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trailer-language.XXXXXX")
trap 'rm -rf "$TR_DIR"' EXIT
cc -O1 -g -DNUVIO_TRAILER_TEST -Isrc \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -ffunction-sections -fdata-sections \
  tests/trailer-language-detail.c src/extras.c src/trailerfonte.c src/js.c \
  -L/opt/homebrew/lib -lSDL2 -Wl,-dead_strip,-undefined,dynamic_lookup \
  -o "$TR_DIR/detail"
"$TR_DIR/detail"
cc -O1 -g -DNV_TPK -DNUVIO_TRAILER_TEST -Isrc \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-return-stack-address -ffunction-sections -fdata-sections \
  tests/home-trailer-timer.c tests/amigosfil_stub.c src/cwordem.c src/cwretido.c \
  src/extras.c src/trailerfonte.c src/js.c \
  -L/opt/homebrew/lib -lSDL2 -Wl,-dead_strip,-undefined,dynamic_lookup \
  -o "$TR_DIR/home"
"$TR_DIR/home"
bash tests/home-trailer-timer.sh
bash tests/trailer-serie.sh
bash tests/trailer-fonte.sh
bash tests/extras-hero.sh
bash tests/trailer-hero-contract.sh
