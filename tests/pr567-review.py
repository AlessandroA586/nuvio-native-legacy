"""Compile actual app identity selection and player scrobble gates in isolation."""
from pathlib import Path
import subprocess
import sys
import tempfile

app = Path('src/app.c').read_text()
player = Path('src/player.c').read_text()
# Registration must follow the synchronous player open, which clears CDN identity.
selected_play = app.split('static void parteTocar(', 1)[1].split('static void abrirParte(', 1)[0]
assert selected_play.index('definirFonteVOD(url)') < selected_play.index('video_tpk_cdn_arquivo('), 'selected-file CDN identity registered before player open'
selection = app.split('static void abrirParte(', 1)[1].split('  stream_definir_atual(fonte);', 1)[0]
resume = app.split('    if (parteFase == PF_RETOMAR) {\n', 1)[1].split('    parteTocar(url, partePos);', 1)[0].rsplit('    }', 1)[0]
start = player.split('{ int ok = fontePrincipal()', 1)[1].split(';   // PRONTO', 1)[0]
pause = player.split('if (tocouS > 0.0f && cs', 1)[1].split(' {', 1)[0]
# Keep the actual duration gate, with harmless preceding predicates removed.
pause = pause[pause.index('duracaoSeg >='):].rstrip().removesuffix(')')
eof = player.split('    int noFim = ', 1)[1].split('\n  }\n  // Com o painel', 1)[0]
duration = player.split('int player_parece_parte(void)', 1)[1].split('// O titulo do aviso', 1)[0]
countdown = player.split('static void durFonteContagem(', 1)[1].split('static void durFonteAtualizar(', 1)[0]
# Compile the real confirmation hook when present; baseline has no cancellation.
confirm = player.split('void player_confirmar_partes(void) {', 1)[1].split('\n}', 1)[0] if 'void player_confirmar_partes(void) {' in player else ''
confirmation = app.split('        partes_sessao_iniciar(alvo, &l, l.atual);', 1)[1].split('        parteArqLista = l;', 1)[0]
code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned Uint32;
#define main partes_existing_tests
#include "tests/partes.c"
#undef main
typedef struct { char tipo[16]; } CatItem;
static CatItem ci = {"series"};
static Stream source;
static int aguardandoFonte, playing, gotT, gotE;
const Stream *stream_item(int n) { (void)n; return &source; }
static const CatItem *cat_item(int n) { (void)n; return &ci; }
static int player_aberto(void) { return playing; }
static int player_indice(void) { return 0; }
static int detail_indice(void) { return 0; }
static int currentT=1, currentE=1;
static void player_episodio_atual(int *t,int *e) { *t=currentT; *e=currentE; }
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
static PartesLista parteArqLista;
static int parteArqK, parteMp4;
static void alvoPlayer(char *alvo, size_t n) { snprintf(alvo,n,"tt-resume"); }
static void parteRecuar(const char *reason) { (void)reason; assert(0); }
static int nomeMp4(const char *name) { (void)name; return 0; }
static void resumeFile(PartesLista l) {
  char alvo[64];
''' + resume + r'''
}
static int fontePrincipal(void) { return 1; }
static int video_pronto(void) { return 1; }
static int video_ativo(void) { return 1; }
static int ehCanal(void) { return 0; }
static int partes;
static int partesAtiva(void) { return partes; }
static double duracaoCatalogo(void) { return 7200; }
static int player_duracao_suspeita(double d,double c) { return d < c/2; }
static int ended = 1, hasNext;
static double clockEof;
static int video_terminou(void) { return ended; }
static double video_pos(void) { return 1800; }
static double monoSeg(void) { return clockEof; }
static int desc_episodios_carregando(int i) { (void)i; return 0; }
static int idxAtual(void) { return 0; }
static int player_proximo_episodio(void) { return hasNext; }
static int checkEof(int meio, int eSerie) {
  int saindo=0, pediuSair=0, scrubbing=0, retomandoSalto=0, aguardaBusca=0;
  double duracaoSeg=1800, posSeg=1800, eofDesde=-1;
  int noFim = ''' + eof + r'''
  return saindo || pediuSair;
}
static Uint32 ticks, durFonteAcaoAte, toastAte, durFonteSkipEm;
static int durFonteCurta, durFonteManter, pedProximaFonte, aberto=1, saindo;
Uint32 SDL_GetTicks(void) { return ticks; }
int player_parece_parte(void)''' + duration + r'''
void player_confirmar_partes(void) {''' + confirm + r'''
}
static void durFonteContagem(''' + countdown + r'''
static void confirmParts(void) {
  const char *alvo="tt-duration";
''' + confirmation + r'''
}
int main(void) {
  int failed=0;
  // A 40-minute part of a 130-minute movie arms rejection at t=1 s.
  PartesLista movie = {.n=3,.atual=0,.arq={
    {.id=1,.tipo=PARTE_EP}, {.id=2,.tipo=PARTE_EP}, {.id=3,.tipo=PARTE_EP}}};
  for (int expired=0; expired<2; expired++) {
    partes_sessao_parar("reset duration case");
    durFonteSkipEm=durFonteAcaoAte=toastAte=9000;
    durFonteManter=1; pedProximaFonte=0; ticks=3000;
    if (expired) durFonteContagem(9000); // queued rejection before app confirmation
    partes_sessao_iniciar("tt-duration", &movie, 0);
    confirmParts();
    durFonteContagem(10000);
    if (player_pediu_proxima_fonte() || durFonteSkipEm || durFonteManter || player_aviso_duracao_ativo()) {
      puts("FAIL confirmed multipart retains duration rejection"); failed=1;
    }
  }
  partes_sessao_parar("duration done");
  durFonteSkipEm=9000; pedProximaFonte=0; durFonteContagem(10000);
  assert(player_pediu_proxima_fonte()==2); // unconfirmed short sources still skip
  if (!failed) puts("PASS multipart confirmation cancels pending and queued rejection");
  /* EOF stays latched while prefetch waits or retries, then final EOF exits. */
  int eofFailed=0;
  for (int retry=0;retry<2;retry++) {
    for (clockEof=0;clockEof<=30;clockEof+=10)
      if (checkEof(1,0)) eofFailed=1;
  }
  if (eofFailed) puts("FAIL intermediate multipart EOF exits while next link waits/retries");
  else puts("PASS intermediate multipart EOF waits for transition");
  failed |= eofFailed;
  assert(checkEof(0,0));
  hasNext=1; assert(!checkEof(0,1)); hasNext=0;
  static PartesLista pack;
  assert(partes_ler_item("{\"id\":7,\"files\":[{\"id\":1,\"name\":\"Show.S00E02.mkv\",\"size\":300000000}]}",NULL,0,&pack));
  assert(pack.arq[0].temporada==0 && pack.arq[0].episodio==2);
  assert(pack.arq[0].tipo==PARTE_EP && partes_principais(&pack)==1);
  static PartesLista specials;
  assert(partes_ler_item("{\"id\":9,\"files\":[{\"id\":1,\"name\":\"Show.S00E01.mkv\",\"size\":300000000},{\"id\":2,\"name\":\"Show.S00E02.mkv\",\"size\":300000000}]}",NULL,0,&specials));
  specials.atual=0;
  assert(partes_principais(&specials)==2);
  for (int season=-1;season<=1;season++) {
    currentT=season; currentE=season<0 ? -1 : 1;
    partes_sessao_parar("resume test");
    resumeFile(specials);
    if (partes_sessao_lista()) {
      puts("FAIL series Resume starts multipart session"); failed=1;
    }
    assert(parteArqK==0 && parteArqLista.arq[0].id==1);
  }
  currentT=currentE=1;
  partes_sessao_parar("resume test");
  strcpy(ci.tipo,"movie"); resumeFile(specials);
  assert(!partes_sessao_lista());
  currentT=currentE=0; resumeFile(specials);
  assert(partes_sessao_lista());
  partes_sessao_parar("resume test done");
  strcpy(ci.tipo,"series"); currentT=currentE=1;
  for (playing=0;playing<2;playing++) {
    selectFile(0,&pack,0);
    if (gotT!=0 || gotE!=2) { puts("FAIL specials-only pack selects S01E02 instead of S00E02"); failed=1; }
  }
  assert(partes_ler_item("{\"id\":8,\"files\":[{\"id\":1,\"name\":\"Movie.CD1.mkv\",\"size\":300},{\"id\":2,\"name\":\"Movie.CD2.mkv\",\"size\":300}]}",NULL,0,&pack));
  assert(partes_principais(&pack)==2 && partes_seguinte(&pack,0)==1);
  assert(!pack.arq[0].temporadaZero);
  PartesLista l = {.n=3,.atual=0,.arq={
    {.temporada=1,.episodio=1,.tipo=PARTE_EP},
    {.temporada=1,.episodio=3,.tipo=PARTE_EP},
    {.tipo=PARTE_ESPECIAL,.extra=1}}};
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
  return failed;
}
'''
# Run identity and gate cases independently so baseline exposes both findings.
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp)/'review.c'
    src.write_text(code)
    includes = ['-I/opt/homebrew/include', '-I/opt/homebrew/include/SDL2'] if sys.platform == 'darwin' else subprocess.check_output(['pkg-config', '--cflags', 'sdl2'], text=True).split()
    subprocess.run(['cc', '-w', '-I.', '-Isrc', *includes, str(src), 'src/partes.c', 'src/js.c',
                    '-lpthread', '-lm', '-o', tmp+'/review'], check=True)
    result = subprocess.run([tmp+'/review'])
    # The explicit unknown sentinel must survive the player's series default.
    default = player.split('// 0/0 e sem episodio;', 1)[1].split('// AS FONTES', 1)[0]
    unknown_guard = 't < 0 && e < 0' in player or 't == -1 && e == -1' in player
    print('PASS' if unknown_guard else 'FAIL', 'player preserves unknown file identity')
    raise SystemExit(result.returncode or (not unknown_guard))
