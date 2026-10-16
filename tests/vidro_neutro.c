// Vidro neutro (dono, 14/10/2026: "o glass UI no frost ta tendo aberracao de
// cor, ficando meio verde"). O fosco lia um assado de OUTRA fonte (a luz da
// paleta do titulo em cena), nao o que esta atras do painel: com um titulo
// verde, o vidro sobre um fundo cinza saia verde. Aqui o fundo e cinza neutro
// e branco, a luz da paleta e verde, e a media da area de vidro tem de sair
// neutra: |G - (R+B)/2| <= 2 em 8 bits. Tambem confere que o vidro desfoca a
// arte ATRAS dele (uma borda preto/branco vira degrade dentro do painel): o
// fundo e uma arte de tela cheia, a fonte do fosco (docs/fosco-nova.md).
//   bash tests/vidro_neutro.sh [pasta-de-capturas]
#include "gfx.h"
#include "corviva.h"
#include "ajustes.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;
static const int LW = 1920, LH = 1080;
static int falhas;

static void confere(int ok, const char *nome) {
  printf("%s: %s\n", ok ? "PASSA" : "FALHA", nome);
  falhas += !ok;
}
static void media(int x, int y, int w, int h, double m[3]) {
  unsigned char *p = malloc((size_t)w * h * 4);
  int i;
  assert(p);
  glFinish(); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glReadPixels(x, LH - y - h, w, h, GL_RGBA, GL_UNSIGNED_BYTE, p);
  m[0] = m[1] = m[2] = 0;
  for (i = 0; i < w * h; i++) { m[0] += p[4*i]; m[1] += p[4*i+1]; m[2] += p[4*i+2]; }
  for (i = 0; i < 3; i++) m[i] /= (double)(w * h);
  free(p);
}
static void salvar(const char *dir, const char *nome) {
  SDL_Surface *s; unsigned char *p, *t; int y; char cam[700];
  if (!dir) return;
  s = SDL_CreateRGBSurface(0, LW, LH, 24, 0xff, 0xff00, 0xff0000, 0);
  glFinish(); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
  p = s->pixels; t = malloc((size_t)s->pitch);
  for (y = 0; y < LH / 2; y++) {
    memcpy(t, p + y * s->pitch, (size_t)s->pitch);
    memcpy(p + y * s->pitch, p + (LH - 1 - y) * s->pitch, (size_t)s->pitch);
    memcpy(p + (LH - 1 - y) * s->pitch, t, (size_t)s->pitch);
  }
  free(t); snprintf(cam, sizeof cam, "%s/%s", dir, nome);
  assert(SDL_SaveBMP(s, cam) == 0); SDL_FreeSurface(s); printf("captura: %s\n", cam);
}
// A full-screen art: left half `l`, right half `r` (gray levels).
static GLuint arte(float l, float r) {
  static unsigned char px[36][64][4];
  GLuint t;
  int x, y;
  for (y = 0; y < 36; y++)
    for (x = 0; x < 64; x++) {
      unsigned char v = (unsigned char)((x < 32 ? l : r) * 255.0f + 0.5f);
      px[y][x][0] = px[y][x][1] = px[y][x][2] = v; px[y][x][3] = 255;
    }
  glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 36, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return t;
}
// O quadro do Frost com "Vidro fosco": a luz da paleta do titulo (verde)
// preparada como antes era assada para o fosco, a forca devolvida a 0, e a
// arte de tela cheia pintada por cima.
static void quadro(float fundo, float direita) {
  static const float verde[4][3] = { {0.10f,0.55f,0.12f}, {0.20f,0.70f,0.18f},
                                     {0.05f,0.45f,0.10f}, {0.15f,0.60f,0.15f} };
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
  gfx_novo_quadro();
  glDisable(GL_SCISSOR_TEST);
  memcpy(nv_ambiente_viva, verde, sizeof verde);
  nv_ambiente_forca = 1.0f; nv_tempo_viva = 0.0f;
  gfx_ambiente_preparar();
  nv_ambiente_forca = 0.0f;
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
  glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
  gfx_rect((GfxRect){ 0, 0, 1920, 1080 }, arte(fundo, direita), GFX_ARTE, 0, 0, 0, 0, 1, 1, 1, 1.0f);
}
static void neutro(const char *nome, double m[3]) {
  double d = m[1] - (m[0] + m[2]) * 0.5;
  char t[200];
  snprintf(t, sizeof t, "%s neutro: media %.1f,%.1f,%.1f  G-(R+B)/2 = %+.2f", nome, m[0], m[1], m[2], d);
  confere(fabs(d) <= 2.0, t);
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : NULL;
  GfxRect painel = { 1240, 140, 560, 760 }, folha = { 160, 140, 560, 760 };
  double m[3];
  int k;
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_Window *w = SDL_CreateWindow("vidro", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  assert(SDL_GL_CreateContext(w));
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  setenv("NUVIO_SHOT_VIDRO_FOSCO", "1", 1); ajustes_teste_vidro_env();
  assert(ajustes_vidro() && ajustes_vidro_fosco());

  // Dois quadros por fundo: o segundo reaproveita o backdrop do primeiro.
  for (k = 0; k < 4; k++) {
    float fundo = k < 2 ? 0.5f : 1.0f;
    const char *nf = k < 2 ? "cinza" : "branco";
    char n[80];
    quadro(fundo, fundo);
    gfx_vidro_painel(painel, 36.0f / painel.h, 0.80f, 1.0f);
    gfx_vidro_folha(folha, 36.0f / folha.h, 1.0f);
    snprintf(n, sizeof n, "painel/%s/q%d", nf, k & 1);
    media((int)painel.x + 40, (int)painel.y + 40, (int)painel.w - 80, (int)painel.h - 80, m); neutro(n, m);
    snprintf(n, sizeof n, "folha/%s/q%d", nf, k & 1);
    media((int)folha.x + 40, (int)folha.y + 40, (int)folha.w - 80, (int)folha.h - 80, m); neutro(n, m);
    if (k == 0) salvar(dir, "vidro-cinza.bmp");
    if (k == 2) salvar(dir, "vidro-branco.bmp");
  }

  // O vidro desfoca o que esta atras: metade esquerda preta, direita branca,
  // a borda em x = 960 no meio do painel. Dentro do painel a luz sobe em
  // rampa dos dois lados da borda (degrade), nao num degrau so em x = 960.
  {
    GfxRect p = { 760, 300, 400, 480 };
    double v[4][3];
    int i, sobe = 1;
    quadro(0.0f, 1.0f);
    gfx_vidro_painel(p, 36.0f / p.h, 0.30f, 1.0f);
    for (i = 0; i < 4; i++) media(944 + 10 * i, 500, 2, 40, v[i]);
    for (i = 0; i < 3; i++) sobe &= v[i + 1][1] > v[i][1] + 2.0;
    printf("borda sob o vidro: %.1f %.1f | %.1f %.1f\n", v[0][1], v[1][1], v[2][1], v[3][1]);
    confere(sobe, "o vidro desfoca o backdrop real (a borda vira degrade)");
    salvar(dir, "vidro-borda.bmp");
  }
  // The fused "Arte borrada" veil must match a separately drawn veil,
  // both on the backdrop and in the frost source. NULL must reset it too.
  {
    const float veils[][4] = { {0.024f, 0.027f, 0.035f, 0.28f},
                              {0.15f, 0.05f, 0.25f, 0.60f}, {0, 0, 0, 0} };
    GLuint source = arte(0.5f, 0.5f);
    int v, pass, c;
    for (v = 0; v < 3; v++) {
      double backdrop[2][3], glass[2][3];
      for (pass = 0; pass < 2; pass++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
        gfx_novo_quadro();
        glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
        gfx_luz_canal_desenhar(source, 1.0f, pass && v < 2 ? veils[v] : NULL, 0);
        if (!pass && v < 2)
          gfx_cor((GfxRect){0, 0, LW, LH}, 0, veils[v][0], veils[v][1], veils[v][2], veils[v][3]);
        media(900, 500, 40, 40, backdrop[pass]);
        gfx_vidro_painel(painel, 36.0f / painel.h, 0.30f, 1.0f);
        media(1450, 500, 40, 40, glass[pass]);
      }
      for (c = 0; c < 3; c++) {
        confere(fabs(backdrop[0][c] - backdrop[1][c]) <= 2.0,
                "Arte borrada: displayed backdrop keeps the same veil");
        confere(fabs(glass[0][c] - glass[1][c]) <= 2.0,
                "Arte borrada: frost source keeps the displayed veil");
      }
    }
    glDeleteTextures(1, &source);
  }
  printf("vidro_neutro: %s\n", falhas ? "FALHA" : "ok");
  return falhas ? 1 : 0;
}
