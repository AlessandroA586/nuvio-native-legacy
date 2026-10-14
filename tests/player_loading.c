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
static int esperandoFonte, tentativaN, tentativaM, reduz, addonLigado;
static char linhaEp[192] = "S01E03 · The Head";
static Stream fonte;
static VideoStats stats;
static char linhas[16][300];
static int n, blur, logo, fills;
static float fill;
float gfx_tex_aspect_atual;
int ajustes_animacoes_reduzidas(void) { return reduz; }
int player_carregando(void) { return 1; }
int ajustes_abertura_addon(void) { return addonLigado; }
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
  // The bar says "loading": no stage text, no percent, with the toggle on or off.
  assert(!tem("Abrindo fonte") && !tem("Carregando") && !tem("%"));
  assert(addonLigado ? tem("AIOStreams") : !tem("AIOStreams"));
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
  for (addonLigado = 0; addonLigado <= 1; addonLigado++) {
    reduz = 0; stats = (VideoStats){0}; video_stats_vazio(&stats);
    tentativaN = tentativaM = 0;
    esperandoFonte = 1;
    quadro(&c); assert(logo == 1 && fills == 0);
    esperandoFonte = 0;
    quadro(&c);
    stats.bufferPct = 0; reduz = 1;
    quadro(&c); assert(fills == 0);
    stats.bufferPct = 100;
    quadro(&c); assert(fills == 1 && fabsf(fill - 200) < .01f);
    stats.bufferPct = 37;
    quadro(&c); assert(fills == 1 && fabsf(fill - 74) < .01f);
    esperandoFonte = 1;
    quadro(&c); assert(fills == 0);
    esperandoFonte = 0; stats.bufferPct = -1;
    stats.bufferSeg = 5; stats.bufferAlvoSeg = 20;
    quadro(&c);
    stats.bufferPct = 101;
    quadro(&c);
    tentativaN = 2; tentativaM = 3;
    c.logo[0] = 0;
    n = logo = 0; desenharCinema(2600, &c, 0);
    assert(logo == 0 && tem("Fallout") && tem("Fonte 2 de 3") == addonLigado);
    snprintf(fonte.provedor, sizeof fonte.provedor, "AIOStreams\n4K\r\t");
    n = 0; desenharCinema(2600, &c, 0);
    assert(!tem("\n") && !tem("\r") && !tem("\t"));
    snprintf(fonte.provedor, sizeof fonte.provedor, "AIOStreams");
    snprintf(c.logo, sizeof c.logo, "local-logo");
  }
  addonLigado = 1;
  fonte.provedor[0] = fonte.rotulo[0] = linhaEp[0] = 0;
  n = 0; desenharCinema(2600, &c, 0);
  assert(!tem("S01E03") && !tem("secret"));
  puts("PASS: S5 shared renderer: backdrop, logo/title, episode, no stage text, addon line only with toggle, real buffer only");
}
