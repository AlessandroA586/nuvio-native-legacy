#!/bin/bash
set -e
cd "$(dirname "$0")/.."
cc tests/queda.c src/queda.c -o /tmp/nuvio-queda-teste -Wall -Wextra
/tmp/nuvio-queda-teste
