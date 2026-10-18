// Bound startup work, inject a rejected glass link, and verify pixels/state.
// TPK must present frames without ever attempting its costly glass program.
#include "gfx.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int links, rejectGlass, slowLinks, glassLinks;
static int isGlass[4096];
static void testLink(GLuint p) {
  links++;
  assert(p < 4096);
  isGlass[p] = 0;   // GL may reuse a rejected program's name
  GLint n = 0;
  GLuint shaders[2];
  glGetAttachedShaders(p, 2, &n, shaders);
  for (int i = 0; i < n; i++) {
    char source[16000];
    glGetShaderSource(shaders[i], sizeof source, NULL, source);
    if (strstr(source, "uRuidoEsc")) isGlass[p] = 1;
  }
  if (isGlass[p]) glassLinks++;
  if (slowLinks) SDL_Delay(30);
  glLinkProgram(p);
}
static void testStatus(GLuint p, GLenum name, GLint *value) {
  glGetProgramiv(p, name, value);
  if (rejectGlass && name == GL_LINK_STATUS && isGlass[p]) *value = GL_FALSE;
}
#ifdef NV_TEST_TPK
#define NV_TPK
#endif
#define glLinkProgram testLink
#define glGetProgramiv testStatus
#include "../src/gfx.c"
#undef glLinkProgram
#undef glGetProgramiv

static void pixel(int x, int y, unsigned char out[4]) {
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out);
}

static int programas(void) {
  int n = 0;
  for (GLuint id = 1; id < 512; id++) n += glIsProgram(id) == GL_TRUE;
  return n;
}

static void screenshot(int pass) {
  const char *prefix = getenv("NV_GLASS_SHOT_PREFIX");
  if (!prefix) return;
  SDL_Surface *image = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  assert(image);
  for (int y = 0; y < 1080; y++)
    glReadPixels(0, 1079 - y, 1920, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                 (unsigned char *)image->pixels + y * image->pitch);
  char path[1024];
  snprintf(path, sizeof path, "%s-%d.png", prefix, pass);
  assert(IMG_SavePNG(image, path) == 0);
  SDL_FreeSurface(image);
}

int main(int argc, char **argv) {
  rejectGlass = argc > 1 && !strcmp(argv[1], "failure");
  slowLinks = argc > 1 && !strcmp(argv[1], "startup");
  SDL_Window *window;
  SDL_GLContext context;
  unsigned char px[4];
  int failed = 0;
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  window = SDL_CreateWindow("Glass lazy link", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window);
  context = SDL_GL_CreateContext(window);
  assert(context);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  Uint32 start = SDL_GetTicks();
  assert(gfx_iniciar());
  Uint32 elapsed = SDL_GetTicks() - start;
  if (slowLinks) {
    int ok = links <= 1 && elapsed < 500;
    printf("%s: startup links=%d elapsed=%u ms (30 ms injected per link)\n", ok ? "PASS" : "FAIL", links, elapsed);
    failed |= !ok;
  }
  slowLinks = 0;
  int antes = programas();
  // Real first frame and host handoff must complete before optional glass.
  gfx_cor((GfxRect){100, 100, 600, 400}, 0, 1, 0, 0, 1);
  SDL_GL_SwapWindow(window);
  for (int pass = 0; pass < 3; pass++) {   // startup, late first use, reuse
    if (pass) for (int f = 0; f < 3; f++) gfx_novo_quadro();
    glClearColor(0.6f, 0.5f, 0.4f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    pixel(400, 1080 - 300, px);   // the backdrop, before the glass
    int fundo = px[0];
    gfx_vidro((GfxRect){ 100, 100, 600, 400 }, 0.1f, GFX_VIDRO_BARRA, 0.0f, 0.0f, 1.0f);
    pixel(400, 1080 - 300, px);   // inside the glass: scrim must darken the backdrop
    int vidro = px[0] < fundo - 40;
    int semLink = pass == 0 || programas() == antes;
    gfx_cor((GfxRect){ 1000, 100, 600, 400 }, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    pixel(1300, 1080 - 300, px);   // the solid red square drawn right after the glass
    int ok = vidro && semLink && px[0] > 250 && px[1] < 5 && px[2] < 5 && glGetError() == GL_NO_ERROR;
    screenshot(pass);
    printf("%s: pass %d, glass=%d programs=%d (start %d), after-glass pixel=%d,%d,%d\n", ok ? "PASS" : "FAIL",
           pass, vidro, programas(), antes, px[0], px[1], px[2]);
    if (!ok) failed = 1;
    antes = programas();
  }
#ifdef NV_TPK
  if (glassLinks != 0) failed = 1;
  // The host remains able to present frames with glass unavailable.
  SDL_GL_SwapWindow(window);
#else
  if (rejectGlass && (glassLinks != 1 || progs[GFX_VIDRO].prog != 0)) failed = 1;
#endif
  if (rejectGlass) for (GLuint p = 1; p < 4096; p++)
    if (isGlass[p] && glIsProgram(p)) failed = 1;
  printf("%s: glass attempts=%d injected-failure=%d; rejected program released\n", failed ? "FAIL" : "PASS", glassLinks, rejectGlass);
  gfx_encerrar();
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return failed;
}
