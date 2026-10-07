#!/bin/bash
# Teste de velocidade, tela nova: latencia/jitter, faixas e veredito
# (src/vazao.c), sem rede. Ver tests/vazao_tabela.c.
#   bash tests/vazao_tabela.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-vazao-tabela.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -Isrc tests/vazao_tabela.c src/vazao.c -o "$work/t" -O1 -g -Wall -Wextra
"$work/t"
