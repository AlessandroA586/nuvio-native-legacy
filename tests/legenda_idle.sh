#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
exec env TEST_SOURCE=tests/legenda_idle.c bash tests/legenda_tamanho.sh
