#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wall -Wno-macro-redefined tests/bibfonte.c -o /tmp/nuvio-bibfonte && /tmp/nuvio-bibfonte
