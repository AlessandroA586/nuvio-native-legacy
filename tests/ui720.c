// Offline 720p UI proof: real Home/Settings, unchanged alpha, isolated blit GPU time.
#include "app.h"
#include "ajustes.h"
#include "addons.h"
#include "fileiras.h"
#include "artehero.h"
#include "catalogo.h"
#include "dados.h"
#include "gfx.h"
#include "gpunivel.h"
#include "home.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint source;
static unsigned char pixels[1920 * 1080 * 4], flipped[sizeof pixels];
static const GfxRect screen = {0, 0, 1920, 1080};

static void readback(void) {
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  assert(glGetError() == GL_NO_ERROR);
}
static void png(const char *dir, const char *scene, const char *mode) {
  char path[1024];
  readback();
  for (int y = 0; y < 1080; y++)
    memcpy(flipped + y * 1920 * 4, pixels + (1079 - y) * 1920 * 4, 1920 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(flipped, 1920, 1080, 32,
                                                     1920 * 4, SDL_PIXELFORMAT_RGBA32);
  assert(s);
  snprintf(path, sizeof path, "%s/%s-%s.png", dir, scene, mode);
  assert(IMG_SavePNG(s, path) == 0);
  SDL_FreeSurface(s);
}
static void frame(int settings) {
  txt_novo_quadro(); tex_novo_quadro(); tex_bombear(16); gfx_novo_quadro();
  gfx_sem_recorte(); gpun_quadro_inicio();
  glClearColor(0.051f, 0.051f, 0.051f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  if (settings) ajustes_desenhar(10000); else home_desenhar(10000);
  gfx_ambiente_descarregar();
  // Interior, rounded boundary and translucent edge all must remain identical.
  gfx_furo((GfxRect){1500, 200, 180, 120});
  gfx_furo_raio((GfxRect){1500, 400, 180, 120}, 24);
  gfx_furo_alfa((GfxRect){1500, 600, 180, 120}, 0.45f);
}
static void copy(int sharp) {
  glDisable(GL_BLEND);
  gfx_tex_aspect_atual = 0;
  gfx_rect(screen, source, GFX_COPIA, 0, sharp ? 1280 : 0,
           sharp ? 720 : 1, 0, 1, 1, 1, 1);
}
static double gradient(const unsigned char *p, int settings) {
  // Fixed text regions: Home title/metadata/synopsis; Settings left labels.
  int x0 = settings ? 180 : 100, x1 = settings ? 600 : 700;
  int y0 = settings ? 180 : 620, y1 = settings ? 950 : 845;
  double sum = 0;
  for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
    int i = ((1079 - y) * 1920 + x) * 4;
    for (int c = 0; c < 3; c++)
      sum += abs(p[i+c] - p[i+4+c]) + abs(p[i+c] - p[i+1920*4+c]);
  }
  return sum / ((x1-x0) * (y1-y0) * 6.0);
}
static double timing(int sharp, GLuint query) {
  double samples[31];
  for (int k = 0; k < 31; k++) {
    GLuint64 ns;
    glBeginQuery(GL_TIME_ELAPSED_EXT, query);
    copy(sharp);
    glEndQuery(GL_TIME_ELAPSED_EXT);
    glGetQueryObjectui64vEXT(query, GL_QUERY_RESULT, &ns);
    samples[k] = ns / 1e6;
  }
  for (int i = 0; i < 31; i++) for (int j = i+1; j < 31; j++)
    if (samples[j] < samples[i]) { double t=samples[i]; samples[i]=samples[j]; samples[j]=t; }
  return samples[15];
}
int main(int argc, char **argv) {
  assert(argc == 2);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_Window *window = SDL_CreateWindow("720p UI proof", 0, 0, 1920, 1080,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(window && SDL_GL_CreateContext(window));
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080); gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar() && txt_iniciar("deploy/app", 1));
  dados_iniciar("deploy/app/art"); ajustes_dir(getenv("NUVIO_DADOS"));
  tex_iniciar(64); gfx_icones_dir("deploy/app/art");
  artehero_definir_falhou(tex_falhou);
  assert(home_iniciar("deploy/app/art"));
  AddonRemoto addon = {.nome="Offline", .url="local", .ativo=1};
  assert(addons_definir_lista(&addon, 1));
  fil_registrar("offline_movie", "Popular movies - Offline catalogue", "Offline", "Filmes", 12);
  assert(fil_definir_tipo("offline_movie", FIL_TIPO_CARTAZ));
  static CatItem items[12];
  CatFileira row = {.ini=0, .n=12};
  snprintf(row.chave, sizeof row.chave, "offline_movie");
  snprintf(row.titulo, sizeof row.titulo, "Popular movies - Offline catalogue");
  snprintf(row.tipo, sizeof row.tipo, "movie");
  for (int i=0; i<12; i++) {
    snprintf(items[i].imdb, sizeof items[i].imdb, "tt90%05d", i);
    snprintf(items[i].tipo, sizeof items[i].tipo, "movie");
    snprintf(items[i].titulo, sizeof items[i].titulo, "The sharpness of small text %d", i+1);
    snprintf(items[i].sinopse, sizeof items[i].sinopse,
             "A local story about distant worlds. Every detail matters when reading from the sofa. "
             "Explore the catalogue, choose a movie and continue watching.");
    // Complete local metadata prevents background /meta requests.
    snprintf(items[i].logo, sizeof items[i].logo, "deploy/app/art/icone-home.png");
    snprintf(items[i].meta, sizeof items[i].meta, "2026 · 120 min");
    snprintf(items[i].poster, sizeof items[i].poster, "deploy/app/art/poster/%02d.jpg", i%8);
    snprintf(items[i].backdrop, sizeof items[i].backdrop, "deploy/app/art/%02d.jpg", i%8);
  }
  cat_definir_tudo(items, 12, &row, 1);
  for (int n=0; n<120; n++) { home_atualizar(1.f/60, 10000); frame(0); }
  assert(ajustes_iniciar());
  gpun_iniciar(1920, 1080);
  glGenTextures(1, &source); glBindTexture(GL_TEXTURE_2D, source);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GLuint query; glGenQueries(1, &query);
  printf("GPU: %s; timer: GL_EXT_timer_query\n", glGetString(GL_RENDERER));
  assert(strstr((const char *)glGetString(GL_EXTENSIONS), "GL_EXT_timer_query"));
  unsigned char *before = malloc(sizeof pixels); assert(before);
  int failed=0;
  for (int settings=0; settings<2; settings++) {
    const char *scene = settings ? "settings" : "home";
    // Same effects as 720p: only render resolution differs in the reference.
    gpun_definir_nivel(2);
    for (int n=0; n<8; n++) { frame(settings); gpun_quadro_fim(); }
    GLint bound;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &bound); assert(bound == 0);
    png(argv[1], scene, "1080p");
    GLuint64 nativeNs, lowNs;
    glBeginQuery(GL_TIME_ELAPSED_EXT, query); frame(settings); gpun_quadro_fim();
    glEndQuery(GL_TIME_ELAPSED_EXT);
    glGetQueryObjectui64vEXT(query, GL_QUERY_RESULT, &nativeNs);
    gpun_definir_nivel(3);
    for (int n=0; n<8; n++) { frame(settings); gpun_quadro_fim(); }
    glBeginQuery(GL_TIME_ELAPSED_EXT, query); frame(settings); gpun_quadro_fim();
    glEndQuery(GL_TIME_ELAPSED_EXT);
    glGetQueryObjectui64vEXT(query, GL_QUERY_RESULT, &lowNs);
    printf("%s whole frame GPU sample: native %.4f ms; 720p %.4f ms\n",
           scene, nativeNs / 1e6, lowNs / 1e6);
    frame(settings);
    glBindTexture(GL_TEXTURE_2D, source);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1280, 720);
    gfx_tex_esquecer(0);
    gpun_quadro_fim();
    png(argv[1], scene, "after");
    readback(); memcpy(before, pixels, sizeof pixels);
    copy(1); readback();
    assert(memcmp(before, pixels, sizeof pixels) == 0); // production uses chosen filter
    copy(0); png(argv[1], scene, "before");
    memcpy(before, pixels, sizeof pixels);
    double old = gradient(before, settings);
    copy(1); readback();
    double improved = gradient(pixels, settings);
    for (size_t i=3; i<sizeof pixels; i+=4) assert(pixels[i] == before[i]);
    assert(pixels[((1079-250)*1920+1550)*4+3] == 0);
    printf("%s gradient: %.6f -> %.6f (%+.2f%%); alpha identical\n",
           scene, old, improved, 100*(improved/old-1));
    if (!(improved > old*1.05)) { fprintf(stderr, "FAIL: %s sharpness gain <5%%\n", scene); failed=1; }
    double linear=timing(0,query), sharp=timing(1,query);
    printf("%s blit GPU median: bilinear %.4f ms; sharp %.4f ms; delta %+.4f ms\n",
           scene, linear, sharp, sharp-linear);
  }
  // Granted Android-sized drawable: no secondary FBO, no final copy.
  gpun_iniciar(1280, 720); gpun_forcar_720();
  glViewport(0, 0, 1280, 720); gfx_tamanho_alvo(1280, 720);
  gfx_novo_quadro();
  gpun_quadro_inicio();
  GLint bound, viewport[4];
  glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &bound); assert(bound == 0);
  glGetIntegerv(GL_VIEWPORT, viewport);
  assert(viewport[2] == 1280 && viewport[3] == 720);
  gpun_quadro_fim(); assert(gfx_n_rect == 0);
  puts("PASS granted 1280x720 drawable: no FBO/blit; native path: no FBO");
  free(before);
  return failed;
}
