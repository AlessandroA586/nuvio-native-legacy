#!/usr/bin/env python3
"""Run the real modal update/row/tab code with deterministic result publication."""
from pathlib import Path
import subprocess
import tempfile

source = Path('src/streams.c').read_text()
rows = source[source.index('static int ptSelecionavel('):source.index('static int partesEvento(')]
rows = rows.replace('static void ptMontar(void) {', 'static void ptMontar(void) { builds++;')
start = source.index('  if (ptAberto) {', source.index('void stream_folha_atualizar('))
update = source[start + len('  if (ptAberto) {'):source.index('    if (ptFoco >= ptNLinhas)', start)]
code = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "partes.h"
static PartesLista ptL;
static int ptNLinhas, ptAba, ptCopias, ptNAbas, ptAbaTipos[3], ptCapLinhas;
static int ptFoco, ptFocoPosto, builds, copies, published = PARTES_PRONTO;
static float ptRolAlvo;
#define PT_LINHA_H 84.0f
enum { PR_ESCOLHER, PR_TEMPORADA, PR_ARQ, PR_COPIA };
typedef struct { int tipo, idx; float y,h; } PtLinha;
static PtLinha *ptLinhas;
static int ptTocando(void) { return -1; }
int partes_resultado(PartesLista *l) {
  if (l) {
    copies++;
    free(l->arq);
    *l = (PartesLista){.estado=published, .n=10000, .atual=0};
    l->arq = calloc(l->n, sizeof *l->arq);
    for (int i=0;i<l->n;i++) l->arq[i].tipo = i%3;
  }
  return published;
}
''' + rows + '\nstatic void frame(void) {\n' + update + r'''
}
int main(void) {
  ptL.estado = PARTES_BUSCANDO;
  frame();
  assert(copies == 1);
  int initial = builds;
  for (int i=0;i<100;i++) frame();
  if (copies != 1 || builds != initial) {
    fprintf(stderr,"FAIL idle pack: copies=%d builds=%d (initial=%d)\n",copies,builds,initial);
    return 1;
  }
  ptIrAba(1);
  assert(builds == initial+1 && copies == 1 && ptAba == 1);
  for (int i=0;i<100;i++) frame();
  assert(builds == initial+1 && copies == 1);
  published = PARTES_BUSCANDO; frame();
  assert(copies == 2 && ptNLinhas == 0);
  published = PARTES_PRONTO; ptFocoPosto = 0; frame();
  assert(copies == 3 && ptNLinhas > 0);
  free(ptL.arq); free(ptLinhas);
  puts("PASS 10,000-file idle pack: no copies or row rebuilds; tab/result changes refresh");
}
'''
# Drawing and navigation consume rows prepared by update or explicit tab/copy changes.
draw = source[source.index('static void desenhaPartes('):source.index('static void desenhaPartes(')+2000]
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'modal.c'
    c.write_text(code)
    subprocess.run(['cc', '-O1', '-Isrc', '-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2', str(c), '-o', str(Path(tmp)/'modal')], check=True)
    subprocess.run([str(Path(tmp)/'modal')], check=True)
assert 'ptMontar();' not in draw, 'draw must reuse prepared rows'
print('PASS draw reuses prepared rows')
