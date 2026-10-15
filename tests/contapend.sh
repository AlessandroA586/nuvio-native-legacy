#!/bin/bash
# Jornal da conta (contapend.c): vistos e Salvos marcados na TV chegam ao
# perfil da conta sem nunca subir lista que apague dado remoto. Servidor falso
# com estado em tests/contapend.c; vistoep/js/jsw de verdade.
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-contapend-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc -O1 -g -Wall -Wextra -Isrc -Wno-deprecated-declarations -Wno-macro-redefined \
  src/contapend.c src/vistoep.c src/js.c src/jsw.c tests/contapend.c \
  -o "$tmp/contapend" -lpthread
"$tmp/contapend"
