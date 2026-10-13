#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc -Wall -Wextra tests/herocat_regras.c -o /tmp/nuvio-herocat-regras && /tmp/nuvio-herocat-regras
