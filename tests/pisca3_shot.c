// "A tela ainda ta piscando" (owner, LG C9): layout Apple TV (Dinamica) with
// Fundo = Frost and frosted glass. Renders consecutive frames of Home and the
// title page while nothing should change (idle, after a focus move, after the
// glass governor drops to smoked glass) and hashes the hero region and the
// whole frame per frame. After every swap the back buffer is filled with
// garbage, as GLES leaves it undefined (EGL_BUFFER_DESTROYED). Flicker = a
// hash that keeps changing in a settled window. PNGs of the first differing
// frames go to argv[1].
#include "ajustes.h"
#include "addons.h"
#include "artehero.h"
#include "catalogo.h"
#include "corviva.h"
#include "dados.h"
#include "detail.h"
#include "gfx.h"
#include "home.h"
#include "ilha.h"
#include "layout.h"
#include "menu.h"
#include "tex_cache.h"
#include "text.h"
#include "trailer.h"
#include "gl_compat.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 1920
#define H 1080
static SDL_Window *win;
static const char *out;
static Uint32 relogio = 10000;
static unsigned char *px;
static unsigned char *ant;
static int failures;
static unsigned char *q1, *q2;   // the two frames before this one
static int nquadro, flashes, sujas, saltos;   // sujas: textures given garbage
static const char *cena = "settle";   // last window started (transitions follow it)

// Mean |diff| over every 7th pixel (RGBA).
static double dif(const unsigned char *a, const unsigned char *b) {
  size_t p; double s = 0; int n = 0;
  for (p = 0; p < (size_t)W * H * 4; p += 28, n++)
    s += abs(a[p] - b[p]) + abs(a[p + 1] - b[p + 1]) + abs(a[p + 2] - b[p + 2]) + abs(a[p + 3] - b[p + 3]);
  return s / n;
}

// Changed box vs the previous frame, layout coords (top-left origin).
// Returns 1 when the change is visible (not 1-LSB easing noise).
static int caixa(const char *nome, int i, int falar) {
  int x, y, x0 = W, y0 = H, x1 = -1, y1 = -1, dmax = 0, n = 0;
  for (y = 0; y < H; y++) for (x = 0; x < W; x++) {
    size_t p = ((size_t)y * W + x) * 4;
    int d = abs(px[p] - ant[p]) + abs(px[p + 1] - ant[p + 1]) + abs(px[p + 2] - ant[p + 2]) + abs(px[p + 3] - ant[p + 3]);
    if (d > dmax) dmax = d;
    if (d) {
      n++;
      if (x < x0) x0 = x;
      if (x > x1) x1 = x;
      if (y < y0) y0 = y;
      if (y > y1) y1 = y;
    }
  }
  if (falar) printf("[pisca3] %s f%03d changed box x %d..%d y %d..%d px=%d maxdiff=%d\n", nome, i, x0, x1, H - 1 - y1, H - 1 - y0, n, dmax);
  return dmax > 8 && n > 100;
}

static void frame(void) {
  Uint32 now = relogio += 16;
  tex_bombear(8);
  home_atualizar(1.0f / 60, now);
  home_trailer_passo(!detail_aberto(), 1.0f / 60, now);   // as app.c
  if (!detail_aberto() && home_pediu_abrir()) {
    HomeItem it;
    if (home_item_focado(&it)) detail_abrir(&it);
  }
  detail_atualizar(1.0f / 60, now);
  corviva_quadro(1.0f / 60, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
  menu_atualizar(1.0f / 60, now);
  txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  gfx_ambiente_preparar();
  glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  gfx_ambiente(1.0f);
  if (!detail_cobre_tela()) home_desenhar(now);
  detail_desenhar(now);
  if (!detail_aberto()) menu_desenhar(now);
  ilha_desenhar(now);
  gfx_ambiente_descarregar();
  glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px);
  // FLASH: the previous frame stands out from both neighbours, which agree.
  if (nquadro >= 2) {
    double a = dif(q2, q1), b = dif(q1, px), c = dif(q2, px);
    if (a > 2.0 && b > 2.0 && c < 0.25 * (a < b ? a : b)) {
      flashes++;
      printf("[pisca3] FLASH %s frame %d: prev-mid %.2f mid-cur %.2f prev-cur %.2f\n", cena, nquadro - 1, a, b, c);
    }
  }
  memcpy(q2, q1, (size_t)W * H * 4); memcpy(q1, px, (size_t)W * H * 4); nquadro++;
  SDL_GL_SwapWindow(win);
  // GLES: the back buffer is undefined after the swap. Garbage it.
  glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

// webOS composites the GL surface over the video plane by its ALPHA: any
// pixel with alpha < 1 shows what is behind (black, or the trailer).
static int furos(void) {
  size_t p;
  int n = 0;
  for (p = 3; p < (size_t)W * H * 4; p += 4) n += px[p] < 250;
  return n;
}

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  if (detail_aberto()) detail_evento(&e); else home_evento(&e);
  frame();
  e.type = SDL_KEYUP;
  if (detail_aberto()) detail_evento(&e); else home_evento(&e);
}

// FNV over GL rows [y0, y1) (origin bottom), every 3rd pixel.
static unsigned hash(int y0, int y1) {
  unsigned h = 2166136261u;
  size_t p;
  for (p = (size_t)y0 * W * 4; p < (size_t)y1 * W * 4; p += 12)
    h = (h ^ px[p] ^ px[p + 1] << 8 ^ px[p + 2] << 16 ^ (unsigned)px[p + 3] << 24) * 16777619u;
  return h;
}

static void png(const char *nome) {
  char path[1024];
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(s);
  for (y = 0; y < H; y++) memcpy((char *)s->pixels + y * s->pitch, px + (size_t)(H - 1 - y) * W * 4, W * 4);
  snprintf(path, sizeof path, "%s/%s.png", out, nome);
  assert(IMG_SavePNG(s, path) == 0);
  SDL_FreeSurface(s);
}

// `n` frames that should all be identical: count frames whose hero or whole
// frame hash differs from the previous one, and A-B-A alternations.
static void parado(const char *nome, int n) {
  unsigned heroAnt = 0, telaAnt = 0, tela2 = 0;
  int i, mudou = 0, alterna = 0, gravou = 0, a0 = gfx_n_assados, furoMax = 0, f0 = gfx_n_fundo_assados;
  char nm[256];
  cena = nome;
  for (i = 0; i < n; i++) {
    frame();
    unsigned hero = hash(H - 640, H), tela = hash(0, H);
    { int f = furos(); if (f > furoMax) furoMax = f; }
    if (i && (hero != heroAnt || tela != telaAnt) && caixa(nome, i, mudou < 6)) {
      mudou++;
      if (i > 1 && tela == tela2) alterna++;
      if (gravou < 2) { snprintf(nm, sizeof nm, "%s-f%03d", nome, i); png(nm); gravou++; }
    }
    memcpy(ant, px, (size_t)W * H * 4);
    tela2 = telaAnt; heroAnt = hero; telaAnt = tela;
  }
  snprintf(nm, sizeof nm, "%s-fim", nome); png(nm);
  printf("[pisca3] %-14s frames=%d changed=%d alternating=%d translucent-px=%d glass-src=%d flat=%d caps=%d\n",
         nome, n, mudou, alterna, furoMax, gfx_vidro_com_fonte, gfx_vidro_chapados, gfx_vidro_capturas);
  printf("[pisca3] %-14s light-bakes=%d bg-bakes=%d forca=%.3f\n", nome, gfx_n_assados - a0, gfx_n_fundo_assados - f0, nv_ambiente_forca);
  printf("%s: %s settled\n", mudou ? "FAIL" : "PASS", nome);
  failures += mudou != 0;
}

// Frames until the whole frame stays identical for 60 frames (at most `max`):
// late network text must not land inside a settled window. Flicker never
// settles, and the window after it still fails.
static void assentar(int max) {
  unsigned h0 = 0;
  int i, iguais = 0;
  for (i = 0; i < max && iguais < 60; i++) {
    unsigned h;
    frame();
    h = hash(0, H);
    iguais = h == h0 ? iguais + 1 : 0;
    h0 = h;
  }
}

// A run that may change (fades): print per-frame mean diff and hole pixels,
// so a jump (one frame far above its neighbours) stands out.
static void rampa(const char *nome, int n) {
  int i;
  double d, dAnt = 0;
  cena = nome;
  for (i = 0; i < n; i++) {
    frame();
    d = dif(ant, px);
    printf("[pisca3] %s f%03d dif=%.2f holes=%d\n", nome, i, d, furos());
    // JUMP: a fade step far above the step before it (a fade only slows down).
    if (i && d > 8.0 && d > 6.0 * dAnt) {
      char nm[96];
      saltos++;
      printf("[pisca3] JUMP %s f%03d: %.2f after %.2f\n", nome, i, d, dAnt);
      snprintf(nm, sizeof nm, "%s-f%03d-jump", nome, i); png(nm);
      memcpy(px, ant, (size_t)W * H * 4);
      snprintf(nm, sizeof nm, "%s-f%03d-before", nome, i); png(nm);
      memcpy(px, q1, (size_t)W * H * 4);
    }
    dAnt = d;
    memcpy(ant, px, (size_t)W * H * 4);
  }
}

// Fake native trailer: plays, settles, then the webOS pipeline stalls
// (video_pronto drops while the uMS reloads) and resumes, then a 4-frame blip.
static void trailerCiclo(const char *onde, int dono) {
  char nm[64];
  GfxRect tela = { 0, 0, NV_TELA_W, NV_TELA_H };
  trailer_teste_falso(1);
  frame(); frame();                           // home: the hero claims the trailer slot
  trailer_abrir("fake://trailer", tela, 0, 0);
  trailer_marcar_dono(dono, "tt9000000");
  snprintf(nm, sizeof nm, "%s-trailer-in", onde); rampa(nm, 120);
  snprintf(nm, sizeof nm, "%s-trailer", onde); parado(nm, 60);
  trailer_teste_falso(0); snprintf(nm, sizeof nm, "%s-stall", onde); rampa(nm, 90);
  trailer_teste_falso(1); snprintf(nm, sizeof nm, "%s-resume", onde); rampa(nm, 90);
  trailer_teste_falso(0); snprintf(nm, sizeof nm, "%s-blip", onde); rampa(nm, 4);
  trailer_teste_falso(1); snprintf(nm, sizeof nm, "%s-blip-back", onde); rampa(nm, 60);
  trailer_fechar(); trailer_teste_falso(0);   // still supported, as on the TV
}

int main(int argc, char **argv) {
  static CatItem items[10];
  CatFileira row;
  char path[1024];
  int i;
  out = argc > 1 ? argv[1] : "/tmp/pisca3";
  px = malloc((size_t)W * H * 4); ant = malloc((size_t)W * H * 4);
  q1 = calloc((size_t)W * H, 4); q2 = calloc((size_t)W * H, 4);
  assert(px && ant && q1 && q2);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);   // as main.c: alpha is the video-plane window
  win = SDL_CreateWindow("pisca3", 0, 0, W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(win && SDL_GL_CreateContext(win));
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, W, H); gfx_tamanho_alvo(W, H);
  { GLint ab = 0; glGetIntegerv(GL_ALPHA_BITS, &ab); printf("[pisca3] alpha bits %d\n", ab); }
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1));
  dados_iniciar("deploy/app/art");
  snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
  FILE *f = fopen(path, "w"); assert(f);
  // Apple TV layout, Fundo = Frost, glass on, no clock (it ticks).
  fputs("homeLayoutLocal 2\nvidroLocal 0\nfundoLocal 2\nidioma 0\ntrailerHero 0\nrelogioTelaLocal 0\nselected_theme 12\n", f);
  fclose(f);
  // V_LIGA: 0 = on. trailer-1310.txt: skip the one-time "trailers off" migration.
  snprintf(path, sizeof path, "%s/trailer-1310.txt", getenv("NUVIO_DADOS"));
  f = fopen(path, "w"); assert(f); fputs("1\n", f); fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  setenv("NUVIO_SHOT_VIDRO_FOSCO", "1", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro() && ajustes_vidro_fosco() && ajustes_home_layout() == HOME_LAYOUT_DINAMICA);
  assert(ajustes_cor_viva() == CORVIVA_SIMPLES);   // C9 log: cor-viva=1
  tex_iniciar(64); gfx_borrao_iniciar(480, 270); gfx_icones_dir("deploy/app/art");
  artehero_definir_falhou(tex_falhou);
  assert(home_iniciar("deploy/app/art")); menu_iniciar(); ilha_relogio_visivel(0);
  { AddonRemoto addon = {.nome = "Offline", .url = "local", .ativo = 1};
    assert(addons_definir_lista(&addon, 1)); }
  memset(&row, 0, sizeof row);
  snprintf(row.chave, sizeof row.chave, "popular_movie");
  snprintf(row.titulo, sizeof row.titulo, "Popular");
  snprintf(row.tipo, sizeof row.tipo, "movie");
  row.n = 10;
  for (i = 0; i < 10; i++) {
    snprintf(items[i].imdb, sizeof items[i].imdb, "tt90%05d", i);
    snprintf(items[i].tipo, sizeof items[i].tipo, "movie");
    snprintf(items[i].titulo, sizeof items[i].titulo, "Offline movie %d", i + 1);
    snprintf(items[i].sinopse, sizeof items[i].sinopse, "An offline story beneath the glass chrome.");
    snprintf(items[i].backdrop, sizeof items[i].backdrop, "deploy/app/art/%02d.jpg", i);
    snprintf(items[i].backdropCatalogo, sizeof items[i].backdropCatalogo, "%s", items[i].backdrop);
    snprintf(items[i].poster, sizeof items[i].poster, "deploy/app/art/poster/%02d.jpg", i);
    snprintf(items[i].logo, sizeof items[i].logo, "deploy/app/art/logo/%02d.png", i);
  }
  cat_definir_tudo(items, 10, &row, 1);

  for (i = 0; i < 240; i++) frame();          // settle: art loads, springs rest
  parado("home-idle", 90);
  trailerCiclo("home", TRAILER_DONO_HOME);
  for (i = 0; i < 120; i++) frame();
  tecla(SDLK_DOWN);
  for (i = 0; i < 150; i++) frame();
  parado("home-moved", 60);
  tecla(SDLK_RIGHT);
  for (i = 0; i < 90; i++) frame();
  { HomeItem it;                              // open the title (carousel, Frost page)
    int k;
    for (k = 0; k < 3 && !home_item_focado(&it); k++) {   // catalogue revision moved: refocus
      tecla(SDLK_LEFT); for (i = 0; i < 60; i++) frame();
      tecla(SDLK_RIGHT); for (i = 0; i < 60; i++) frame();
    }
    assert(home_item_focado(&it)); detail_abrir(&it); }
  for (i = 0; i < 300; i++) frame();
  assentar(900);                              // network meta (Cinemeta, IMDb) lands late
  assert(detail_aberto());
  parado("detail-idle", 90);
  trailerCiclo("detail", TRAILER_DONO_DETALHE);   // C9 log: "[trailer] apple tt1430132 ... HLS"
  for (i = 0; i < 120; i++) frame();
  tecla(SDLK_DOWN);                           // into the page: the card expands over Frost
  for (i = 0; i < 200; i++) frame();
  parado("page-idle", 90);
  tecla(SDLK_DOWN);
  for (i = 0; i < 600; i++) frame();          // the offline extras text lands late
  parado("page-down", 90);
  for (i = 0; i < 4 * 150; i++) gfx_vidro_governar(100.0, 16.7);   // C9: the governor trips
  for (i = 0; i < 30; i++) frame();
  parado("detail-smoked", 90);
  printf("[pisca3] jumps=%d\n%s: trailer fades without a jump\n", saltos, saltos ? "FAIL" : "PASS");
  failures += saltos != 0;
  printf("[pisca3] garbage-textures=%d\n", sujas);
  printf("[pisca3] flashes=%d\n%s: no single-frame flash\n", flashes, flashes ? "FAIL" : "PASS");
  failures += flashes != 0;
  printf("[pisca3] degraded=%d gl=%#x\n", gfx_vidro_degradado(), glGetError());
  tex_encerrar(); SDL_Quit(); free(px);
  return failures ? 1 : 0;
}

// GLES: a new texture's contents are undefined. Garbage, as a Mali can give.
#undef glTexImage2D
extern void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void pisca3_teximage(GLenum t, GLint l, GLint i, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty,
                     const void *p) {
  unsigned char *lixo;
  size_t n, k;
  glTexImage2D(t, l, i, w, h, b, f, ty, p);
  if (p || ty != GL_UNSIGNED_BYTE || (f != GL_RGB && f != GL_RGBA) || w <= 0 || h <= 0) return;
  n = (size_t)w * h * (f == GL_RGB ? 3 : 4);
  lixo = malloc(n);
  if (!lixo) return;
  for (k = 0; k < n; k++) lixo[k] = (unsigned char)(k * 2654435761u >> 13) | 0x80;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexSubImage2D(t, l, 0, 0, w, h, f, ty, lixo);
  sujas++;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  free(lixo);
}
