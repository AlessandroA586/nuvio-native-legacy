#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc tests/vertudo_pag.c -Isrc -o /tmp/nuvio-vertudo-pag -O1 -g
/tmp/nuvio-vertudo-pag
