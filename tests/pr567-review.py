"""Compile actual app identity selection and player scrobble gates in isolation."""
from pathlib import Path
import subprocess
import tempfile

app = Path('src/app.c').read_text()
player = Path('src/player.c').read_text()
selection = app.split('static void abrirParte(', 1)[1].split('  stream_definir_atual(fonte);', 1)[0]
start = player.split('{ int ok = fontePrincipal()', 1)[1].split(';   // PRONTO', 1)[0]
pause = player.split('if (tocouS > 0.0f && cs', 1)[1].split(' {', 1)[0]
# Keep the actual duration gate, with harmless preceding predicates removed.
pause = pause[pause.index('duracaoSeg >='):].rstrip().removesuffix(')')
code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned Uint32;
typedef struct { int dolbyVision; } Stream;
typedef struct { int temporada, episodio, tipo, extra; } ParteArq;
typedef struct { int n, atual; ParteArq arq[3]; } PartesLista;
typedef struct { double pos; } PartesPonto;
enum { PARTE_EP, PARTE_ESPECIAL, PARTE_EXTRA };
typedef struct { char tipo[16]; } CatItem;
static CatItem ci = {"series"};
static Stream source;
static int aguardandoFonte, playing, gotT, gotE;
static const Stream *stream_item(int n) { (void)n; return &source; }
static const CatItem *cat_item(int n) { (void)n; return &ci; }
static int player_aberto(void) { return playing; }
static int player_indice(void) { return 0; }
static int detail_indice(void) { return 0; }
static void player_episodio_atual(int *t,int *e) { *t=1; *e=1; }
static void detail_ep_foco(int *t,int *e) { *t=1; *e=1; }
static void player_encerrar(void) {}
static void limparFonteVOD(void) {}
static void parteZerar(void) {}
static void consumirFolhaTocar(int *t,int *e) { *t=1; *e=1; }
static void diagnostico_cancelar_vazao(void) {}
static void player_abrir(int n,void *p) { (void)n; (void)p; }
static void player_definir_episodio(int t,int e) { gotT=t; gotE=e; }
static void selectFile(''' + selection + r'''
}
static int fontePrincipal(void) { return 1; }
static int video_pronto(void) { return 1; }
static int video_ativo(void) { return 1; }
static int ehCanal(void) { return 0; }
static int partes;
static int partesAtiva(void) { return partes; }
static double duracaoCatalogo(void) { return 7200; }
static int player_duracao_suspeita(double d,double c) { return d < c/2; }
int main(void) {
  PartesLista l = {.n=3,.atual=0,.arq={{1,1,PARTE_EP,0},{1,3,PARTE_EP,0},{0,0,PARTE_ESPECIAL,1}}};
  for (playing=0;playing<2;playing++) {
    selectFile(0,&l,1);
    if (gotT!=1 || gotE!=3) { puts("FAIL selected S01E03 inherits S01E01"); return 1; }
    selectFile(0,&l,2);
    if (gotT!=-1 || gotE!=-1) { puts("FAIL unnumbered OVA inherits episode"); return 1; }
    l.arq[2].episodio=2;
    selectFile(0,&l,2);
    assert(gotT==0 && gotE==2);
    l.arq[2].episodio=0;
    l.arq[1].temporada=0; selectFile(0,&l,1); assert(gotT==1 && gotE==3); l.arq[1].temporada=1;
  }
  strcpy(ci.tipo,"movie"); selectFile(0,&l,1); assert(gotT==1 && gotE==1);
  float duracaoSeg=1800; int tocando=1,scrubbing=0,saindo=0,erroFonte=0,esperandoFonte=0;
  for (partes=0;partes<2;partes++) {
    int start = fontePrincipal()''' + start + r''';
    int pause = ''' + pause + r''';
    if (start!=partes || pause!=partes) { puts("FAIL multipart progress/start/pause gate"); return 1; }
  }
  puts("PASS selected identity and multipart start/pause gates");
}
'''
# Run identity and gate cases independently so baseline exposes both findings.
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp)/'review.c'
    src.write_text(code)
    subprocess.run(['cc', '-w', str(src), '-o', tmp+'/review'], check=True)
    result = subprocess.run([tmp+'/review'])
    # The explicit unknown sentinel must survive the player's series default.
    default = player.split('// 0/0 e sem episodio;', 1)[1].split('// AS FONTES', 1)[0]
    unknown_guard = 't < 0 && e < 0' in player or 't == -1 && e == -1' in player
    print('PASS' if unknown_guard else 'FAIL', 'player preserves unknown file identity')
    raise SystemExit(result.returncode or (not unknown_guard))
