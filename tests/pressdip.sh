#!/bin/bash
# Press-dip do OK: o card afunda enquanto a tecla esta apertada. Ver tests/pressdip.c.
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra -O2 tests/pressdip.c -Isrc -o /tmp/nuvio-pressdip-test -lm
/tmp/nuvio-pressdip-test
rm -f /tmp/nuvio-pressdip-test
grep -q 'pressdip_passo' src/home.c || { echo "FALHA: home.c nao usa pressdip_passo"; exit 1; }
