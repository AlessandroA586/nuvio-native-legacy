#!/bin/bash
# Execute the real Library event handlers without GL, accounts or network.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-biblioteca-up.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
python3 - "$tmp" <<'PYTHON'
from pathlib import Path
import os, re, subprocess, sys
source = (subprocess.check_output(['git', 'show', os.environ['BIB_REV'] + ':src/biblioteca.c'], text=True)
          if os.environ.get('BIB_REV') else Path('src/biblioteca.c').read_text())
tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')

def function(signature):
    start = re.search(r'(?m)^' + re.escape(signature), source).start()
    depth = 0
    for token in tokens.finditer(source, start):
        if token[0] == '{':
            depth += 1
        elif token[0] == '}':
            depth -= 1
            if not depth:
                return source[start:token.end()] + '\n'
    raise AssertionError(signature)

constants = re.findall(r'(?m)^#define BIB_FIL_\w+\s+\d+.*$', source)
constants += re.findall(r'enum \{ (?:MODO_|TIPO_|ORD_|VIS_|FONTE_|EST_)[^}]+\};', source)
handlers = ''.join(function(signature) for signature in (
    'static int estado(', 'static void eventoAberta(',
    'static void biblioteca_evento_(', 'int biblioteca_quer_sair(', 'int biblioteca_pediu_relogio('))
handlers = handlers.replace('biblioteca_evento_', 'biblioteca_evento', 1)
Path(sys.argv[1], 'biblioteca_eventos.inc').write_text('\n'.join(constants) + '\n' + handlers)
PYTHON
for platform in tv android; do
  flags=()
  [ "$platform" != android ] || flags+=(-DNV_ANDROID)
  cc -std=c11 -O1 -Wall -Wextra ${flags[@]+"${flags[@]}"} tests/biblioteca_up.c src/focus.c \
    -I"$tmp" -Isrc -I/opt/homebrew/include/SDL2 -o "$tmp/test"
  "$tmp/test"
done
