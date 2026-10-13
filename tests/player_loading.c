// Shared TV renderer, offline: actual stage, safe source line, honest buffer.
#include "abrecine.h"
#include "catalogo.h"
#include "streams.h"
#include "video.h"
#include "gfx.h"
#include "text.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define NV_TELA_W 1920.0f
#define NV_TELA_H 1080.0f
static AbreCine cine = { .viva = 1, .veu = 1, .foco = 1, .desde = 1000 };
static int esperandoFonte, tentativaN, tentativaM, reduz;
static char linhaEp[192] = "S01E03 · The Head";
static Stream fonte;
static VideoStats stats;
static char linhas[16][300];
static int n, blur, logo, fills;
static float fill;
float gfx_tex_aspect_atual;
int ajustes_animacoes_reduzidas(void) { return reduz; }
int player_carregando(void) { return 1; }
static void corFocoPlayer(float *r, float *g, float *b) { *r = .8f; *g = .5f; *b = .2f; }
const char *i18n(const char *s) { return s; }
const Stream *stream_item(int idx) { (void)idx; return &fonte; }
int stream_atual(void) { return 0; }
void video_estatisticas(VideoStats *s) { *s = stats; }
const char *artehero_url(const CatItem *c) { return c->backdrop; }
const char *artehero_logo_sessao(const CatItem *c) { return c->logo; }
GLuint tex_obter_hero(const char *s) { return *s ? 1 : 0; }
GLuint tex_obter_larg(const char *s, float w) { (void)w; return *s ? 1 : 0; }
GLuint tex_obter_larg_qualquer(const char *s, float w) { (void)w; return *s ? 2 : 0; }
float tex_aspecto(const char *s) { (void)s; return 2; }
int tex_marca_escura(const char *s) { (void)s; return 0; }
GLuint gfx_desfocado(GLuint t, const char *s) { (void)s; blur++; return t; }
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float f, float px, float py, float rad,
              float cr, float cg, float cb, float ca) { if (t == 2) logo++; }
void gfx_cor(GfxRect r, float rad, float cr, float cg, float cb, float ca) {
  if (r.h == 3 && cr == .8f) { fills++; fill = r.w; }
}
TxtLinha txt_linha(TxtEstilo es, const char *s, int r, int g, int b, int a) {
  assert(n < 16); snprintf(linhas[n++], sizeof linhas[0], "%s", s);
  return (TxtLinha){ .w = (int)strlen(s) * 12, .h = 24 };
}
TxtLinha txt_linha_corta(TxtEstilo es, const char *s, int r, int g, int b, int a, float w) {
  return txt_linha(es, s, r, g, b, a);
}
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) {}
#include "render.c"
static int tem(const char *s) {
  for (int i = 0; i < n; i++) if (strstr(linhas[i], s)) return 1;
  return 0;
}
static void quadro(CatItem *c) {
  n = blur = logo = fills = 0; fill = 0;
  desenharCinema(2600, c, 0);
  assert(blur == 1);
  assert(tem("S01E03"));
  assert(tem("AIOStreams"));
  assert(!tem("RAW") && !tem("secret") && !tem("http"));
}
int main(void) {
  CatItem c = {0};
  snprintf(c.backdrop, sizeof c.backdrop, "local-backdrop");
  snprintf(c.logo, sizeof c.logo, "local-logo");
  snprintf(c.titulo, sizeof c.titulo, "Fallout");
  snprintf(fonte.provedor, sizeof fonte.provedor, "AIOStreams");
  snprintf(fonte.rotulo, sizeof fonte.rotulo, "RAW.FILE\nhttps://example.invalid/?secret=1");
  snprintf(fonte.url, sizeof fonte.url, "https://example.invalid/?secret=2");
  video_stats_vazio(&stats);
  esperandoFonte = 1;
  quadro(&c); assert(logo == 1 && tem("Abrindo fonte") && !tem("%"));
  esperandoFonte = 0;
  quadro(&c); assert(tem("Carregando") && !tem("%"));
  stats.bufferPct = 37; reduz = 1;
  quadro(&c); assert(tem("37%") && fills == 1 && fabsf(fill - 74) < .01f);
  esperandoFonte = 1;
  quadro(&c); assert(!tem("37%") && fills == 0);
  esperandoFonte = 0; stats.bufferPct = -1;
  stats.bufferSeg = 5; stats.bufferAlvoSeg = 20;
  quadro(&c); assert(!tem("%")); // seconds are not startup percent
  stats.bufferPct = 101;
  quadro(&c); assert(!tem("%"));
  tentativaN = 2; tentativaM = 3;
  c.logo[0] = 0;
  quadro(&c); assert(logo == 0 && tem("Fallout") && tem("Fonte 2 de 3"));
  fonte.provedor[0] = fonte.rotulo[0] = 0;
  n = 0; desenharCinema(2600, &c, 0); assert(!tem("secret"));
  puts("PASS: S5 shared renderer: backdrop, logo/title, episode, stages, safe source, real buffer only");
}
