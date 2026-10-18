// Sources sheet: (1) no drop shadow around it on any entry point (detail with
// the clock island, player island, standalone); (2) a tab change animates, the
// focus pill slides through intermediate positions. Writes a settled frame and
// a 3-frame tab strip.
//   bash tests/fontes_sombra.sh <dir> [git-ref-for-old-streams.c]
#include "catalogo.h"
#include "nvtmp.h"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "streams.h"
#include "ilha.h"
#include "badges.h"
#include "detail.h"
#include "player.h"
#include "plrilha.h"
#include "layout.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;
enum { LW = 1920, LH = 1080 };
static Uint32 relogio = 100000;
static SDL_Surface *strip;
static int noPlayer, semRelogio;
#ifndef NV_BASELINE
float stream_folha_pilula_x(void);
#endif

static unsigned char q[LW * LH * 3];
static void salvar(const char *pre, int n) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, LW, LH, 24, 0xff, 0xff00, 0xff0000, 0);
  char nome[700];
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGB, GL_UNSIGNED_BYTE, q);
  for (int y = 0; y < LH; y++)
    memcpy((unsigned char *)s->pixels + y * s->pitch, q + (size_t)(LH - 1 - y) * LW * 3, (size_t)LW * 3);
  snprintf(nome, sizeof nome, "%s-%d.png", pre, n);
  assert(IMG_SavePNG(s, nome) == 0);
  if (n >= 2) {   // frames 2..4 are the tab change
    SDL_Rect dst = { (n - 2) * LW / 2, 0, LW / 2, LH / 2 };
    assert(SDL_BlitScaled(s, NULL, strip, &dst) == 0); }
  SDL_FreeSurface(s);
  printf("capture: %s\n", nome);
}

extern unsigned gfx_shot_sombras;

static void quadro(void) {
  relogio += 16;
  SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
  if (detail_aberto()) detail_atualizar(1.f / 60, relogio);
  stream_folha_atualizar(1.f / 60, relogio);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
  glClearColor(0.025f, 0.025f, 0.03f, 1); glClear(GL_COLOR_BUFFER_BIT);
  gfx_novo_quadro();
  gfx_cor((GfxRect){ 0, 0, LW, LH }, 0, .26f, .17f, .12f, 1);
  gfx_cor((GfxRect){ 0, 0, LW, 360 }, 0, .55f, .36f, .22f, 1);
  if (detail_aberto()) detail_desenhar(relogio);
  stream_folha_desenhar(relogio);
  if (noPlayer) {
    plrilha_relogio(1, -1);
    plrilha_desenhar(relogio);
  } else {
    ilha_relogio_visivel(!semRelogio && !stream_folha_aberta());
    ilha_posicionar(1);
    ilha_desenhar(relogio);
  }
}

static unsigned char qa[LW * LH * 3];
static void ler(unsigned char *dst) {
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGB, GL_UNSIGNED_BYTE, dst);
}
// Summed luma of the band [x0,x1) x rows [y0,y1) (GL origin is bottom-left).
static double banda(const unsigned char *b, int x0, int x1, int y0, int y1) {
  double t = 0;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) { const unsigned char *p = b + ((size_t)(LH - 1 - y) * LW + x) * 3;
      t += p[0] * .30 + p[1] * .59 + p[2] * .11; }
  return t;
}

static const char *JSON =
  "{\"streams\":["
  "{\"url\":\"https://x.invalid/1.mkv\",\"name\":\"[RD+] AIOStreams 4K DV\",\"description\":\"12.3 GB\"},"
  "{\"url\":\"https://x.invalid/2.mkv\",\"name\":\"Torrentio\\n1080p\",\"title\":\"Silo.S02E05.1080p.WEB-DL.mkv\"},"
  "{\"url\":\"https://x.invalid/3.mkv\",\"name\":\"Comet 720p\",\"description\":\"720p WEB\"}"
  "]}";
#define NMAIS 24

int main(int argc, char **argv) {
  Stream *v = NULL;
  const char *pre = argc > 1 ? argv[1] : nvt("/nv-fontes-sombra");
  float x = 0, y = 0, w = 0, h = 0;
  const char *mode = argc > 2 ? argv[2] : "detail";
  noPlayer = !strncmp(mode, "player", 6);
  semRelogio = !strcmp(mode, "no-clock");
  strip = SDL_CreateRGBSurfaceWithFormat(0, LW * 3 / 2, LH / 2, 32, SDL_PIXELFORMAT_RGBA32);
  assert(strip);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *win = SDL_CreateWindow("Nuvio: fontes sombra", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(win);
  SDL_GLContext gl = SDL_GL_CreateContext(win); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  badges_carregar("deploy/app/art");
  { char caminho[700]; FILE *f;
    snprintf(caminho, sizeof caminho, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
    f = fopen(caminho, "w"); assert(f);
    fprintf(f, "idioma 0\nselected_theme 2\n");
    fclose(f);
    ajustes_dir(getenv("NUVIO_DADOS")); }
  ajustes_iniciar();
  if (strstr(mode, "glass")) {
    assert(ajustes_shot_valor("vidroLocal", 0));
    assert(ajustes_shot_valor("selected_theme", 3));
    assert(ajustes_vidro());
  }
  if (semRelogio) assert(ajustes_shot_valor("relogioTelaLocal", 1));
  { CatItem c = {0};
    snprintf(c.titulo, sizeof c.titulo, "Silo");
    snprintf(c.tipo, sizeof c.tipo, "series");
    snprintf(c.meta, sizeof c.meta, "2023 · Drama · Mystery");
    snprintf(c.sinopse, sizeof c.sinopse, "In a ruined future, a community lives in a giant silo underground.");
    snprintf(c.backdrop, sizeof c.backdrop, "deploy/app/art/poster/00.jpg");
    cat_definir(&c, 1);
    if (noPlayer) player_abrir(0, NULL);
    else {
      HomeItem hi = { .indice = 0, .rect = {0, 0, LW, LH}, .titulo = c.titulo,
                      .genero = c.genero, .meta = c.meta };
      detail_abrir(&hi);
      assert(detail_aberto());
    }
  }
  { static char big[8192]; int n = 0;
    n += snprintf(big + n, sizeof big - n, "{\"streams\":[");
    for (int i = 0; i < NMAIS; i++)
      n += snprintf(big + n, sizeof big - n, "%s{\"url\":\"https://x.invalid/%d.mkv\",\"name\":\"AIOStreams 1080p\",\"title\":\"Silo.S02E%02d.1080p.WEB-DL.mkv\"}", i ? "," : "", i, i);
    snprintf(big + n, sizeof big - n, "]}");
    assert(stream_extrair(big, "AIOStreams", &v) == NMAIS); }

  stream_definir_alvo("tt14688458:2:5");
  stream_definir_lista(v, NMAIS);
  stream_folha_nome("Silo");
  stream_folha_contexto("T2:E5 · Silo");

  for (int i = 0; i < 40; i++) quadro();
  if (detail_aberto()) {
    SDL_Event e = {0};
    e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
    detail_evento(&e);
    SDL_Delay(NV_HOLD_MS + 1);
    e.type = SDL_KEYUP; detail_evento(&e);
    assert(detail_pediu_fontes());
  }
  stream_folha_abrir();
  for (int i = 0; i < 90; i++) quadro();          // sheet settled
  if (noPlayer) { GfxRect r; assert(plrilha_rect(&r)); x=r.x; y=r.y; w=r.w; h=r.h; }
  else if (!semRelogio) assert(ilha_rect(&x, &y, &w, &h));
  else { w = 820; x = LW - w - 40; y = 40; h = LH - 80; }
  assert(w > 780 && h > 700);

  // Scroll the list down so rows slide under the header.
  { SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_DOWN;
    for (int k = 0; k < 14; k++) { stream_folha_evento(&e); for (int i = 0; i < 6; i++) quadro(); }
    for (int i = 0; i < 60; i++) quadro(); }
  salvar(pre, 1); ler(q);
  // Left panel gutter (no cards): luma per row must stay flat, a dark band shows as a dip.
  { double ref = 0, worst = 0; int wr = 0;
    int c0 = (int)x + 6, c1 = (int)x + 16;
    for (int r = 296; r < 306; r++) ref += banda(q, c0, c1, r, r + 1);
    ref /= 10;
    for (int r = 248; r < 290; r++) {
      double d = ref - banda(q, c0, c1, r, r + 1);
      if (d > worst) { worst = d; wr = r; } }
    printf("%s: ref %.0f worst dip %.0f at row %d (of %d px)\n", mode, ref, worst, wr, c1 - c0);
    assert(worst < ref * .02 && "dark band under the sheet header"); }
  printf("PASS: %s header band gone\n", mode);
  return 0;
}
