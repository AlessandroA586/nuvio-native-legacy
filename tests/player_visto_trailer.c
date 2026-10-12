// #205 "VISTOS FALSOS": a duracao do TRAILER nao pode valer pela do titulo.
//
// O caminho, provado em log de TV (tcl-agora.log, 412.txt) e no #385: o detalhe
// toca o trailer do IMDb pela MESMA pipeline de video do player (trailer.c:
// video_tocar), o prepare do trailer demora, e o evento PRONTO dele — 129 s —
// chega DEPOIS de o dono pedir Reproduzir, sem selo de sessao
// (nv_tpk_video_evento: "tpk evento 1 (129036)"). Ele cai na sessao nova e
// vira a duracao do TITULO (duracaoSeg = 129 s num filme de 106 min). O filme
// real toca; a 69 s de reproducao real a regra da conclusao (pos >= dur-60 e
// pos >= dur/2) da o filme por VISTO, pos := duracaoSeg grava 100% no
// catalogo, o trakt.c manda /scrobble/stop 100%, e visto_titulo marca o filme
// no Simkl e na conta. "Quase todos os filmes da fileira Trending Movies"
// marcados sem ninguem ter visto nada.
//
// Aqui o pipeline e falso (mesmo truque de player_fonte_duracao.c: nenhuma
// janela, TV ou rede) e o teste fabrica o veneno na mao: video_duracao()
// devolve os 129 s do trailer enquanto a "pessoa" assiste segundos reais do
// filme. Os destinos da marcacao viram contadores (renome no .sh): sem token,
// sem rede — e a prova e que NENHUM destino e chamado. Os dois ultimos casos
// sao os controles ao contrario: titulo real assistido de verdade tem que
// continuar marcando, e o filme abandonado no comeco tem que continuar
// gravando o progresso REAL.
#include "player.h"
#include "catalogo.h"
#include "dados.h"
#include "linguas.h"
#include "trakt.h"
#include "visto.h"
#include "vistoep.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "trailer.h"
#include "video.h"
#include "plrui.h"
#include "text.h"
#include "layout.h"
#include "ajustes.h"
#include "trailerfonte.h"

// Relogio monotono controlado: seek/retomada nao ganham tempo assistido.
static double agoraMono;
int nv_test_clock_gettime(clockid_t id, struct timespec *ts) {
  (void)id;
  ts->tv_sec = (time_t)agoraMono;
  ts->tv_nsec = (long)((agoraMono - ts->tv_sec) * 1e9);
  return 0;
}

// Pipeline falso: pos e dur sob controle total do teste.
static double dur, pos;
static int ativo, pausado, buffer, terminou;
static char urlAtual[4096];
int video_tocar(const char *url) {
  snprintf(urlAtual, sizeof urlAtual, "%s", url);
  ativo = 1; pausado = buffer = terminou = 0; pos = 0; return 1;
}
void video_parar(void) { ativo = 0; }
void video_pausar(int p) { pausado = p; }
void video_buscar(double p) { pos = p; }
int video_pausa_confirmada(void) { return 0; }
int video_tocando(void) { return ativo && !pausado; }
unsigned video_bufferando_ms(void) { return (unsigned)buffer; }
int video_pronto(void) { return ativo; }
int video_ativo(void) { return ativo; }
int video_falhou(void) { return 0; }
int video_terminou(void) { return terminou; }
int video_conflito_recurso(void) { return 0; }
double video_pos(void) { return pos; }
double video_duracao(void) { return dur; }
const char *video_url_atual(void) { return urlAtual; }
static const char *const fonte = "https://example.invalid/teste.mp4";
int video_iniciar_auto(void) { return 1; }

// OS DESTINOS, virados contadores pelo renome do .sh. A marcacao de vistos
// parte daqui (fecharSessao) para o Trakt (scrobble stop) e para o
// Simkl/conta (visto_titulo) — nenhuma rede aqui, so a contagem.
static int nTrakt, nVisto;
void trakt_marcar(const char *imdb, double posSeg, double durSeg, double assistidoSeg) {
  (void)imdb; (void)posSeg; (void)durSeg; (void)assistidoSeg; nTrakt++;
}
int visto_titulo(const char *imdb, const char *tipo, const int *temporadas,
                 int nt, int visto, int destinos) {
  (void)imdb; (void)tipo; (void)temporadas; (void)nt; (void)visto; (void)destinos;
  nVisto++;
  return 1;
}
int visto_episodios(const char *imdb, const char *tipo, const VistoPar *pares,
                    int n, int visto, int destinos) {
  (void)imdb; (void)tipo; (void)pares; (void)n; (void)visto; (void)destinos;
  nVisto++;
  return 1;
}

// Mantem os headers do SDK macOS; seleciona so a implementacao nativa real.
#undef __APPLE__
#include "../src/trailer.c"
static void quadro(void) { player_atualizar(.016f, SDL_GetTicks()); }

// Abre, assiste `p` segundos de reproducao real com o pipeline informando `d`
// de duracao (a do trailer, quando e o veneno), e sai — o momento em que
// fecharSessao decide o que grava.
static void abrir(int filme, const char *imdb, const char *meta,
                  const char *durEp, double d, double p) {
  CatItem c = {0}; CatEp ep = {0};
  strcpy(c.titulo, "Fixture"); strcpy(c.imdb, imdb);
  strcpy(c.tipo, filme ? "movie" : "series");
  if (meta) snprintf(c.meta, sizeof c.meta, "%s", meta);
  cat_definir(&c, 1);
  if (durEp) {
    ep.temporada = ep.episodio = 1;
    snprintf(ep.duracao, sizeof ep.duracao, "%s", durEp);
    cat_definir_episodios(0, &ep, 1);
  }
  nTrakt = nVisto = 0;
  player_abrir(0, NULL);
  if (durEp) player_definir_episodio(1, 1);
  dur = d;
  player_definir_fonte(fonte);
  quadro();
  // Dois quadros: o primeiro pode buscar o ponto salvo (retomada); depois
  // disso a posicao e a que a pessoa assistiu de verdade.
  // Reproducao continua real, amostrada uma vez por segundo.
  for (int i = 0; i < (int)p; i++) { agoraMono += 1; pos += 1; quadro(); }
  pos = p; quadro(); quadro();
  player_encerrar();
}

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0); dados_iniciar(""); ling_local_legenda("none");

  // Sem minutagem: fonte pronta na ultima posicao, sem reproducao real.
  // Antes: fecharSessao arredonda 69/129 e chama TODOS os destinos.
  CatItem semMeta = {0};
  strcpy(semMeta.imdb, "tt90000001"); strcpy(semMeta.tipo, "movie");
  cat_definir(&semMeta, 1); dur = 129.036;
  player_abrir(0, fonte); quadro(); pos = 69; quadro();
  player_encerrar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // (1) O RELATO DO #205, filme: meta diz 1 h 46 min, o pipeline (PRONTO do
  // trailer) diz 129 s, a pessoa assiste 69 s e sai. A regra antiga: 69 >=
  // 129-60 e 69 >= 129/2 — CONCLUIU. Nao pode concluir: 69 s nao sao 90% de
  // 106 minutos.
  abrir(1, "tt22687790", "2023 · 1 h 46 min", NULL, 129.036, 69.0);
  assert(nTrakt == 0);                   // nenhum scrobble, nem pause
  assert(nVisto == 0);                   // nem Simkl, nem conta
  assert(cat_item(0)->progresso == 0);   // 69/129 = 53% de mentira, nada.

  // (2) A MESMA SESSAO, quem ficou mais tempo: passou ate ALEM da duracao
  // falsa. Qualquer saida depois de 69 s concluia — assistir 2 min de um
  // filme de 106 min nao e assisti-lo.
  abrir(1, "tt22687791", "1 h 46 min", NULL, 129.036, 130.0);
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // (3) O EPISODIO do #385, com minutagem na lista: o mesmo PRONTO atrasado.
  // A conclusao ja era segurada pela duracao suspeita, mas o PROGRESSO era
  // gravado com a duracao do trailer (69 s de 129 = 53% do episodio).
  abrir(0, "tt22687792", NULL, "43 min", 129.036, 69.0);
  assert(nTrakt == 0 && nVisto == 0);
  assert(vistoep_estado("tt22687792", 1, 1) != 1);   // sem o check local
  assert(cat_item(0)->progresso == 0);

  // (4) CONTROLE POSITIVO, filme assistido de verdade ate o fim: a duracao do
  // pipeline bate com o meta. Tem que continuar marcando em todo destino.
  abrir(1, "tt22687793", "1 h 46 min", NULL, 6357.0, 6300.0);
  assert(nTrakt == 1 && nVisto == 1);
  assert(cat_item(0)->progresso == 100);

  // (5) CONTROLE POSITIVO, episodio ate o fim.
  abrir(0, "tt22687794", NULL, "43 min", 2580.0, 2525.0);
  assert(nTrakt == 1);
  assert(vistoep_estado("tt22687794", 1, 1) == 1);
  assert(cat_item(0)->progresso == 100);

  // (6) CONTROLE: filme real abandonado no comeco — progresso REAL gravado,
  // scrobble de progresso (pause) enviado, nada marcado como visto.
  abrir(1, "tt22687795", "1 h 46 min", NULL, 6357.0, 200.0);
  assert(nTrakt == 1 && nVisto == 0);
  assert(cat_item(0)->progresso > 0 && cat_item(0)->progresso < 10);

  // Seek ate o fim nao e tempo assistido, mesmo com a minutagem certa.
  semMeta.progresso = 0; strcpy(semMeta.imdb, "tt90000002");
  cat_definir(&semMeta, 1); nTrakt = nVisto = 0; dur = 2580;
  player_abrir(0, fonte); quadro(); agoraMono += 200; pos = 2525; quadro();
  player_encerrar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // Um trailer substitui a pipeline de uma sessao principal ainda aberta.
  // Passa pelo trailer_abrir REAL; ao fechar a sessao nao pode copiar seu fim.
  strcpy(semMeta.imdb, "tt90000003"); cat_definir(&semMeta, 1);
  player_abrir(0, fonte); quadro();
  for (int i = 0; i < 130; i++) { agoraMono += 1; pos += 1; quadro(); }
  dur = 180;
  trailer_abrir("https://example.invalid/trailer.mp4", (GfxRect){0,0,100,100}, 0, 0);
  assert(trailer_aberto());
  pos = 180; quadro(); player_encerrar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);
  trailer_fechar();

  // Trailer nativo termina, fecha e deixa os valores antigos na pipeline.
  // A sessao perdeu a fonte; mesmo restaurar a URL nao recupera sua prova.
  strcpy(semMeta.imdb, "tt90000005"); cat_definir(&semMeta, 1); dur = 2580;
  player_abrir(0, fonte); quadro();
  for (int i = 0; i < 2400; i++) { agoraMono += 1; pos += 1; quadro(); }
  trailer_abrir(fonte, (GfxRect){0,0,100,100}, 0, 0);
  dur = pos = 180; terminou = 1;
  trailer_atualizar(SDL_GetTicks()); assert(!trailer_aberto());
  quadro(); player_encerrar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // Pausa e buffer nao contam.
  strcpy(semMeta.imdb, "tt90000004"); cat_definir(&semMeta, 1); dur = 2580;
  player_abrir(0, fonte); quadro();
  video_pausar(1);
  for (int i = 0; i < 180; i++) { agoraMono += 1; quadro(); }
  video_pausar(0); buffer = 1;
  for (int i = 0; i < 180; i++) { agoraMono += 1; pos += 1; quadro(); }
  buffer = 0; pos = 2525; quadro(); player_encerrar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // Amostras em degraus de 200ms a 60fps: nao perder 95% do tempo real.
  strcpy(semMeta.imdb, "tt90000006"); cat_definir(&semMeta, 1); dur = 180;
  player_abrir(0, fonte); quadro(); quadro();
  for (int i = 1; i <= 170 * 60; i++) {
    agoraMono += 1.0 / 60.0; pos = (i / 12) * 0.2; quadro();
  }
  player_encerrar();
  assert(nTrakt == 1 && nVisto == 1 && cat_item(0)->progresso == 100);

  // Sem fonte (preparo/pre-busca): o video e de um trailer, nao do player.
  nTrakt = nVisto = 0; strcpy(semMeta.imdb, "tt90000007"); cat_definir(&semMeta, 1);
  player_abrir(0, NULL); dur = 180;
  trailer_abrir(fonte, (GfxRect){0,0,100,100}, 0, 0);
  for (int i = 0; i < 180; i++) { agoraMono += 1; pos += 1; quadro(); }
  player_encerrar(); trailer_fechar();
  assert(nTrakt == 0 && nVisto == 0 && cat_item(0)->progresso == 0);

  // Regressao sem catalogo: 69s reais com duracao falsa de 129s nao concluem.
  abrir(1, "tt90000008", NULL, NULL, 129.036, 69);
  assert(nTrakt == 1 && nVisto == 0 && cat_item(0)->progresso < 90);

  puts("PASS: trailer, seek, pausa e buffer nao marcam; reproducao real grava progresso e visto");
  return 0;
}
