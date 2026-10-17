#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
# Linka somente a lógica exercitada, sem inicializar SDL/AppKit nem vídeo.
# Além de ser rápido, permite ASAN no macOS beta sem o diálogo do loader SDL.
#
# src/fileiras.c ENTRA NA LISTA porque home_layout.c chama sincronizarFileiras,
# que passou a consultar fil_unir/fil_limite quando a escolha local de fileiras
# nasceu. A lista e escrita a mao, entao ela nao acompanha sozinha: o teste
# ficou sem linkar ("_fil_unir referenced from _sincronizarFileiras") e o
# script morria antes de rodar um caso sequer.
cc ${flags[@]+"${flags[@]}"} src/catalogo.c src/progresso.c src/focus.c src/ajustes.c src/posterprov.c src/redeurl.c src/colecoes.c src/js.c src/catordem.c src/fileiras.c src/artehero.c src/corviva.c tests/home_tex_duble.c src/cwordem.c src/cwretido.c src/curadoria.c tests/home_layout.c tests/amigosfil_stub.c \
  -Isrc -o ${TMPDIR:-/tmp}/nuvio-home-tests -O1 -g -ffunction-sections -fdata-sections \
  -DCURADORIA_TESTE -Wl,-dead_strip -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-home-tests
# Exercise the production suppression guard, including Up from the first Split row.
python3 - <<'PY'
from pathlib import Path
import re
import subprocess
import tempfile

source = Path('src/home.c').read_text()
guard = re.search(r'if \(([^;{}]+)\) aTexto = 0\.0f;', source)
assert guard, 'hero copy suppression guard missing'
with tempfile.TemporaryDirectory() as directory:
    test = Path(directory) / 'hero-focus.c'
    test.write_text('''
#include <assert.h>
enum { FILEIRA_NORMAL, FILEIRA_HEROI_DIVIDIDO };
int main(void) {
  struct { int tipo; } fileiras[1];
  struct { int fileira; } foco;
  int nFileiras = 1;
  for (int tipo = FILEIRA_NORMAL; tipo <= FILEIRA_HEROI_DIVIDIDO; tipo++)
    for (int focoHero = 0; focoHero <= 1; focoHero++)
      for (int row = -1; row <= nFileiras; row++) {
        fileiras[0].tipo = tipo;
        foco.fileira = row;
        float aTexto = 1.0f;
        ''' + guard.group(0) + '''
        assert(aTexto == ((!focoHero && row == 0 &&
                            tipo == FILEIRA_HEROI_DIVIDIDO) ? 0.0f : 1.0f));
      }
}
''')
    executable = str(Path(directory) / 'hero-focus')
    subprocess.run(['cc', str(test), '-o', executable], check=True)
    subprocess.run([executable], check=True)
print('hero focus suppression: ok')
PY
