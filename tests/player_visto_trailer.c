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

// Pipeline falso: pos e dur sob controle total do teste.
static double dur, pos;
static int ativo;
int video_tocar(const char *url) { (void)url; ativo = 1; pos = 0; return 1; }
void video_parar(void) { ativo = 0; }
void video_pausar(int p) { (void)p; }
void video_buscar(double p) { pos = p; }
int video_pausa_confirmada(void) { return 0; }
int video_tocando(void) { return ativo; }
int video_pronto(void) { return ativo; }
int video_ativo(void) { return ativo; }
int video_falhou(void) { return 0; }
int video_conflito_recurso(void) { return 0; }
double video_pos(void) { return pos; }
double video_duracao(void) { return dur; }
const char *video_url_atual(void) { return "https://example.invalid/teste.mp4"; }

// OS DESTINOS, virados contadores pelo renome do .sh. A marcacao de vistos
// parte daqui (fecharSessao) para o Trakt (scrobble stop) e para o
// Simkl/conta (visto_titulo) — nenhuma rede aqui, so a contagem.
static int nTrakt, nVisto;
void trakt_marcar(const char *imdb, double posSeg, double durSeg) {
  (void)imdb; (void)posSeg; (void)durSeg; nTrakt++;
}
int visto_titulo(const char *imdb, const char *tipo, const int *temporadas,
                 int nt, int visto, int destinos) {
  (void)imdb; (void)tipo; (void)temporadas; (void)nt; (void)visto; (void)destinos;
  nVisto++;
  return 1;
}

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
  player_definir_fonte(video_url_atual());
  quadro();
  // Dois quadros: o primeiro pode buscar o ponto salvo (retomada); depois
  // disso a posicao e a que a pessoa assistiu de verdade.
  pos = p; quadro(); quadro();
  player_encerrar();
}

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0); dados_iniciar(""); ling_local_legenda("none");

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

  puts("PASS: duracao de trailer nao marca visto nem grava progresso; titulo real continua marcando");
  return 0;
}
