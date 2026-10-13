/* #484: advance playback without remote events and observe the real player
 * draw path, with controlled pipeline time and counted text draws; no GL. */
#include "text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char *const labels[] = { "Early", "Five minutes", "Twenty five minutes" };
static const char *drawn;
static float drawnAlpha;
static TxtLinha testLine(TxtEstilo e, const char *s, int r, int g, int b,
                         int a, float max, TxtFamilia f, int emphasis) {
  (void)e; (void)r; (void)g; (void)b; (void)a; (void)max; (void)f; (void)emphasis;
  for (unsigned i = 0; i < sizeof labels / sizeof labels[0]; i++)
    if (!strcmp(s, labels[i]))
      return (TxtLinha){ .tex = i + 1, .w = (int)strlen(s) * 20, .h = 50 };
  return (TxtLinha){0};
}
static void testDraw(TxtLinha l, float x, float y, float alpha) {
  (void)x; (void)y;
  if (l.tex) {
    assert(l.tex <= sizeof labels / sizeof labels[0]);
    drawn = labels[l.tex - 1]; drawnAlpha = alpha;
  }
}
static int testBidi(TxtFamilia f, TxtEstilo e, const char *s, char *out, size_t n) {
  (void)f; (void)e; (void)s; (void)out; (void)n;
  return 0;
}
#include "video.h"
#include "gfx.h"
#include <time.h>
static double playback, monotonic;
static double testPosition(void) { return playback; }
static int testReady(void) { return 1; }
static double testDuration(void) { return 7200; }
static const char *testUrl(void) { return "fixture.mp4"; }
static int testClock(clockid_t id, struct timespec *ts) {
  (void)id;
  ts->tv_sec = (time_t)monotonic;
  ts->tv_nsec = (long)((monotonic - ts->tv_sec) * 1e9);
  return 0;
}
static void testHole(GfxRect r) { (void)r; }
#define gfx_furo testHole
#define clock_gettime testClock
#define video_pos testPosition
#define video_ativo testReady
#define video_pronto testReady
#define video_tocando testReady
#define video_duracao testDuration
#define video_url_atual testUrl
#define txt_linha_corta_enfase testLine
#define txt_desenhar_alpha testDraw
#define txt_bidi_legenda testBidi
#include "../src/player.c"
#undef txt_linha_corta_enfase
#undef txt_desenhar_alpha
#undef txt_bidi_legenda

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  /* No provider, account or GL window. The pipeline position and monotonic
   * clock advance together; addon files use this same parser and draw path. */
  aberto = tocando = visivel = 1;
  entrada = anim = cheio = fileira = 1;
  esperandoFonte = erroFonte = canalSessao = 0;
  comVideo = retomadaAplicada = 1;
  snprintf(fonteSessao, sizeof fonteSessao, "%s", testUrl());
  trailerSessao = trailer_geracao();
  duracaoSeg = 7200;
  ultimoInput = inicioImagem = 1000;
  gfx_opacidade_grupo = 0; /* suppress unrelated GL calls */
  legEstilo = (VideoLegendaEstilo){ .tamanho=120, .posicao=3 };
  const char *documents[] = {
    "1\n00:00:01,000 --> 00:00:03,000\nEarly\n\n"
    "2\n00:05:00,000 --> 00:05:02,000\nFive minutes\n\n"
    "3\n00:25:00,000 --> 00:25:02,000\nTwenty five minutes\n\n",
    "WEBVTT\n\n00:00:01.000 --> 00:00:03.000\nEarly\n\n"
    "00:05:00.000 --> 00:05:02.000\nFive minutes\n\n"
    "00:25:00.000 --> 00:25:02.000\nTwenty five minutes\n\n"
  };
  for (unsigned format = 0; format < sizeof documents / sizeof documents[0]; format++) {
    posSeg = 0;
    visivel = 1; anim = 1;
    relogio_zerar(&relLeg);
    legenda_definir_corpo(documents[format]);
    for (unsigned second = 1; second <= 1502; second++) {
      Uint32 now = 1000 + second * 1000;
      playback = second;
      monotonic = 100 + playback;
      player_atualizar(.1f, now);
      if (second == 2 || second == 301 || second == 1501) {
        const char *expected = second == 2 ? "Early" :
                               second == 301 ? "Five minutes" : "Twenty five minutes";
        drawn = ""; drawnAlpha = 0;
        if (second == 2) desenharLegendaExterna();
        else player_desenhar(now);
        printf("%s at %.2fs: '%s', alpha=%.2f, controls=%d\n",
               format ? "VTT" : "SRT", posSeg, drawn, drawnAlpha, visivel);
        assert(!strcmp(drawn, expected) && drawnAlpha > .99f);
        if (second > 4) assert(!visivel && anim <= .005f);
      }
    }
    assert(ultimoInput == 1000);
    legenda_desligar();
  }
  SDL_Quit();
  puts("PASS: external SRT/VTT drawn after 25 minutes without remote input");
}
