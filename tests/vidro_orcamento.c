// Glass budget per frame with frost on: the top bar (track + avatar), the
// clock island and a glass panel over a hero / detail art backdrop, with the
// rows scrolling under them, the hero crossfading to another art and the
// detail backdrop. The frost reads the art pre-blurred once (gfx_desfocado,
// docs/fosco-nova.md): zero screen captures and zero blur passes on every
// frame, while the panel still shows the art's colour through the frost.
//   bash tests/vidro_orcamento.sh
#include "gfx.h"
#include "ajustes.h"
#include "dados.h"
#include "ilha.h"
#include "menu.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const int LW = 1920, LH = 1080;
static int falhas;

static void confere(int ok, const char *nome) {
  printf("%s: %s\n", ok ? "PASSA" : "FALHA", nome);
  falhas += !ok;
}

// A saturated horizontal ramp, a -> b: the art behind the glass.
static GLuint arte(int r0, int b0) {
  static unsigned char px[36][64][4];
  GLuint t;
  int x, y;
  for (y = 0; y < 36; y++)
    for (x = 0; x < 64; x++) {
      px[y][x][0] = (unsigned char)(r0 + (b0 - r0) * x / 63); px[y][x][1] = 60;
      px[y][x][2] = (unsigned char)(b0 + (r0 - b0) * x / 63); px[y][x][3] = 255;
    }
  glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 36, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return t;
}

// One Home/detail frame: the art backdrop (hero crossfading A -> B by `mix`,
// or the detail art), the rows scrolled `rola` px, the bar, the island and a
// glass panel over the rows.
static GLuint artA, artB;
static float mix;
static int detalhe;
static void quadro(float rola, Uint32 agora) {
  GfxRect tela = { 0, 0, 1920, 1080 };
  int i;
  gfx_novo_quadro();
  glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
  if (detalhe) gfx_rect(tela, artB, GFX_DETALHE, 0.4f, 0, 0, 0.0f, 0, 0, 0, 1.0f);
  else {
    if (mix < 1.0f) gfx_rect(tela, artA, GFX_HERO, 0, 0, 0, 0, 0, 0, 0, 1.0f);
    if (mix > 0.0f) gfx_rect(tela, artB, GFX_HERO, 0, 0, 0, 0, 0, 0, 0, mix);
  }
  for (i = 0; i < 12; i++)
    gfx_cor((GfxRect){ 0, 400 + i * 90.0f - rola, 1920, 60 }, 0, .1f + .06f * i, .2f, .5f - .03f * i, 1.0f);
  menu_atualizar(0.016f, agora);
  menu_desenhar(agora);
  ilha_desenhar(agora);
  gfx_vidro_painel((GfxRect){ 1240, 140, 560, 220 }, 36.0f / 600.0f, 0.80f, 1.0f);
}

// Worst captures / passes over `n` frames, and whether the frost showed the
// art's colour (flat glass is neutral gray, R = G = B) at the panel centre.
static void fase(const char *nome, int n, int k0, float rola0, float drola, float mix0, float dmix) {
  int k, cap = 0, pas = 0, cor = 99;
  unsigned char p[4];
  for (k = 0; k < n; k++) {
    mix = mix0 + dmix * k;
    if (mix > 1.0f) mix = 1.0f;
    quadro(rola0 + drola * k, 16u * (k0 + k));
    if (gfx_vidro_capturas > cap) cap = gfx_vidro_capturas;
    if (gfx_vidro_passadas > pas) pas = gfx_vidro_passadas;
    glReadPixels(1520, LH - 250, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    if (abs((int)p[0] - (int)p[2]) < cor) cor = abs((int)p[0] - (int)p[2]);
  }
  printf("%s: up to %d captures and %d passes per frame, min |R-B| %d\n", nome, cap, pas, cor);
  confere(cap == 0 && pas == 0, nome);
  confere(cor >= 4, "frost shows the art behind the panel");
}

int main(void) {
  static char dir[] = "/tmp/nuvio-vidro-orc-XXXXXX";
  char cam[700];
  FILE *f;
  int k;
  GLuint fbo, tex;
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("vidro", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
  glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);

  assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dir);
  f = fopen(cam, "w"); assert(f);
  fputs("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\nidioma 0\n", f); fclose(f);
  ajustes_dir(dir);
  setenv("NUVIO_SHOT_VIDRO_FOSCO", "1", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro() && ajustes_vidro_fosco() && ajustes_menu_topo());
  menu_iniciar();
  menu_pilula_mostrar(1.0f);
  ilha_relogio_visivel(1);

  artA = arte(230, 30); artB = arte(30, 230);
  // Warm-up: the bar springs settle in ~2 s.
  for (k = 0; k < 150; k++) quadro(0.0f, 16u * k);
  fase("home rows scrolling over the hero", 20, 150, 4.0f, 4.0f, 0.0f, 0.0f);
  fase("hero crossfading to another art", 30, 170, 80.0f, 0.0f, 0.05f, 0.05f);
  fase("still home", 5, 200, 80.0f, 0.0f, 1.0f, 0.0f);
  detalhe = 1;
  fase("detail backdrop, page scrolling", 20, 205, 80.0f, 6.0f, 1.0f, 0.0f);

  unlink(cam);
  { char t[700]; snprintf(t, sizeof t, "%s/ajustes.tmp", dir); unlink(t); }
  printf("vidro_orcamento: %s\n", falhas ? "FALHA" : "ok");
  return falhas ? 1 : 0;
}
