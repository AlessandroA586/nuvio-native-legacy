// LIVE TV SUBTITLES (owner, 03/10: "a legenda ta errada, ta pegando e uma de
// filmes"). The addon subtitle list in addons.c is GLOBAL: it holds whatever
// the last movie/episode asked for (detail open, player open). Opening a
// channel never asks for subtitles again (tocarCanal in app.c), so the
// Subtitles sheet of a live channel listed the previous movie's OpenSubtitles
// results. On a live channel only the stream's own tracks count.
#include "../src/faixas.c"
#include <assert.h>
static Legenda filme[3];
static int nEmb;
static VideoFaixa emb[2];
static const char *canal = "";
int video_n_legenda(void) { return nEmb; }
const VideoFaixa *video_legenda(int i) { return i >= 0 && i < nEmb ? &emb[i] : NULL; }
int mkvass_estado(void) { return 0; }
int video_n_audio(void) { return 1; }
int mkvass_varredura(void) { return 0; }
int addons_n_legendas(void) { return 3; }
const Legenda *addons_legenda(int i) { return i >= 0 && i < 3 ? &filme[i] : NULL; }
const char *player_id_canal(void) { return canal; }
const char *i18n(const char *s) { return s; }
static VideoLegendaEstilo estilo;
VideoLegendaEstilo *player_leg_estilo(void) { return &estilo; }
void player_leg_estilo_mudou(void) {}
void player_leg_estilo_tocou(int m) { (void)m; }
int assrender_ativo(void) { return 0; }
int assrender_texto_simples(void) { return 0; }
int main(void) {
  // #439: every added 10% step is selectable before wrapping to 50%.
  estilo.tamanho = 250;
  for (int pct = 260; pct <= 300; pct += 10) {
    ciclarEstilo(0); assert(estilo.tamanho == pct);
  }
  ciclarEstilo(0); assert(estilo.tamanho == 50);
  // Style arrow and OK paths share the ±60 s bound, with the existing 250 ms step.
  passoAtrasoEstilo(60); assert(estilo.atrasoMs == 15000);
  passoAtrasoEstilo(300); assert(estilo.atrasoMs == LEGUI_ATRASO_MAX);
  ciclarEstilo(7); assert(estilo.atrasoMs == LEGUI_ATRASO_MAX);
  passoAtrasoEstilo(-600); assert(estilo.atrasoMs == -LEGUI_ATRASO_MAX);
  ciclarEstilo(7); assert(estilo.atrasoMs == -LEGUI_ATRASO_MAX + 250);
  snprintf(filme[0].rotulo, sizeof filme[0].rotulo, "Portugues (OpenSubtitles)");
  // A movie: embedded + addon subtitles, as before.
  assert(nLegendas() == 3);
  nEmb = 1;
  assert(nLegendas() == 4 && nLinhas(1) == 5);
  // A live channel: the movie list left in memory must not show up.
  canal = "xtream:1:42";
  nEmb = 0;
  assert(nLegendas() == 0 && nLinhas(1) == 1);   // only "Nenhuma"
  nEmb = 2;                                       // closed captions of the stream
  assert(nLegendas() == 2 && nLinhas(1) == 3);
  puts("live channel subtitles: only the stream's own tracks, no addon list ok");
  return 0;
}
