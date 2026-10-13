#!/bin/bash
# Contagem "voce viu N de M". Sem SDL, rede ou disco.
set -eu
cd "$(dirname "$0")/.."
cc src/vistocont.c tests/vistocont.c -Isrc -o /tmp/nuvio-vistocont-tests -O1 -g -Wall -Wextra
/tmp/nuvio-vistocont-tests
