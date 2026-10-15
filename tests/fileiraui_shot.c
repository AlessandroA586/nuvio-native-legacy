// CAPTURA DO MODAL DE ESTILO DA FILEIRA E DA FILEIRA REDONDA, SEM REDE (artes
// de deploy/app/art). Base: tests/colestilos_shot.c.
//
//   <pref>-1-estilo-redonda   modal de estilo de uma fileira de pessoas, foco
//                             na forma Redonda (a previa e o disco)
//   <pref>-2-ordem            modal de um grupo de colecao, foco na Ordem
//   <pref>-3-home-redonda     a Home com a fileira de pessoas em Redonda
//
//   bash tests/fileiraui_shot.sh /tmp/nv-fileiraui [prefixo]
#include "ajustes.h"
#include "nvtmp.h"
#include "addons.h"
#include "artehero.h"
#include "catalogo.h"
#include "colecoes.h"
#include "corviva.h"
#include "ctxmenu.h"
#include "dados.h"
#include "fileiras.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include "posterprov.h"
#include <string.h>
#include <assert.h>

static SDL_Window *janela;
static const char *dirDados;

static void gravar(const char *bmp) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && s);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(s, bmp) == 0);
  SDL_FreeSurface(s);
  free(pix);
}

static void quadros(int n, const char *bmp) {
  int i;
  for (i = 0; i < n; i++) {
    Uint32 agora = SDL_GetTicks();
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, agora);
    corviva_quadro(1.0f / 60.0f, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
    txt_novo_quadro();
    tex_novo_quadro();
    gfx_novo_quadro();
    gfx_ambiente_preparar();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_ambiente(1.0f);
    home_desenhar(agora);
    ctx_atualizar(1.0f / 60.0f, agora);
    ctx_desenhar(agora);
    if (bmp && i == n - 1) gravar(bmp);
    SDL_GL_SwapWindow(janela);
    SDL_Delay(8);
  }
}

static void teclaHome(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  home_evento(&e);
  e.type = SDL_KEYUP;
  home_evento(&e);
}

static void teclaCtx(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  ctx_evento(&e);
  e.type = SDL_KEYUP;
  ctx_evento(&e);
  quadros(6, NULL);
}

static void ajusta(void) {
  char cam[700];
  FILE *a;
  snprintf(cam, sizeof cam, "%s/ajustes.txt", dirDados);
  a = fopen(cam, "w");
  assert(a);
  fprintf(a, "idioma 0\nhomeLayoutLocal 0\nvidroLocal 1\n"
             "modernLandscapePostersEnabled 1\nselected_theme 0\n");
  fclose(a);
  ajustes_dir(dirDados);
  artehero_fundo_addon(ajustes_fundo_addon());
  posterprov_preferir_addon(ajustes_poster_addon());
  col_arte_conta(ajustes_col_arte_conta());
}

static const char *JS =
  "{\"collections\":[{\"id\":\"cs\",\"title\":\"Streaming\","
  "\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":["
  "{\"id\":\"a\",\"title\":\"Com capa\",\"coverImageUrl\":\"deploy/app/art/03.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
  "{\"id\":\"b\",\"title\":\"So capa\",\"coverImageUrl\":\"deploy/app/art/05.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}";

#define PESSOAS "tmdb.people_popular"
static const char *NOMES[] = { "Greta Gerwig", "Denis Villeneuve", "Bong Joon-ho",
  "Ava DuVernay", "Wes Anderson", "Martin Scorsese", "Chloé Zhao", "Jordan Peele" };

static void focaFileira(const char *titulo) {
  int i;
  for (i = 0; i < 14; i++) teclaHome(SDLK_UP);
  for (i = 0; i < 12 && !strstr(home_rastro_foco(), titulo); i++) {
    teclaHome(SDLK_DOWN);
    quadros(4, NULL);
  }
  assert(strstr(home_rastro_foco(), titulo));
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : nvt("/nv-fileiraui/fil");
  static CatItem itens[12];
  static CatFileira fils[2];
  SDL_GLContext gl;
  char bmp[800], cache[700];
  int i, k;

  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (!dirDados || !strstr(dirDados, "nuvio-fileiraui-shot")) {
    fprintf(stderr, "recuse: NUVIO_DADOS tem de ser a pasta temporaria do teste\n");
    return 1;
  }
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: estilo da fileira", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1920, 1080,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_borrao_iniciar(480, 270);
  artehero_definir_falhou(tex_falhou);
  snprintf(cache, sizeof cache, "%s/cache", dirDados);
  tex_cache_dir(cache);
  gfx_icones_dir("deploy/app/art");
  ajusta();
  assert(home_iniciar("deploy/app/art"));

  { AddonRemoto addon = { .nome = "Fixture", .url = "http://127.0.0.1:1/manifest.json", .ativo = 1 };
    assert(addons_definir_lista(&addon, 1)); }
  memset(itens, 0, sizeof itens);
  memset(fils, 0, sizeof fils);
  snprintf(fils[0].chave, sizeof fils[0].chave, "continue_watching");
  snprintf(fils[0].titulo, sizeof fils[0].titulo, "Continuar assistindo");
  snprintf(fils[0].tipo, sizeof fils[0].tipo, "movie");
  fils[0].ini = 0; fils[0].n = 4;
  snprintf(fils[1].chave, sizeof fils[1].chave, PESSOAS);
  snprintf(fils[1].titulo, sizeof fils[1].titulo, "Diretores populares");
  snprintf(fils[1].tipo, sizeof fils[1].tipo, "movie");
  snprintf(fils[1].base, sizeof fils[1].base, "https://addon.invalid/x");
  snprintf(fils[1].catId, sizeof fils[1].catId, "people_popular");
  fils[1].ini = 4; fils[1].n = 8;
  for (i = 0; i < 12; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt90%03d", i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    if (i < 4) {
      snprintf(c->titulo, sizeof c->titulo, "Titulo Fixture %d", i);
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", i);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", i);
      c->progresso = 15 + i * 20;
    } else {
      snprintf(c->titulo, sizeof c->titulo, "%s", NOMES[i - 4]);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/elenco/%02d_%d.jpg", (i - 4) / 6, (i - 4) % 6);
      snprintf(c->backdrop, sizeof c->backdrop, "%s", c->poster);
    }
  }
  assert(col_definir_json(JS) == 2);
  cat_definir_tudo(itens, 12, fils, 2);
  quadros(60, NULL);

  // 1. O modal de estilo da fileira de pessoas, foco na Redonda.
  focaFileira("Diretores populares");
  ctx_abrir_fileira(PESSOAS, "Diretores populares");
  quadros(30, NULL);
  { FilEstiloLinha l[FIL_TIPO_N];
    int n = fil_estilo_linhas(PESSOAS, l, FIL_TIPO_N), alvo = -1;
    for (k = 0; k < n; k++) if (l[k].n == 1 && l[k].tipos[0] == FIL_TIPO_REDONDA) alvo = k;
    assert(alvo >= 0);
    for (k = 0; k < alvo; k++) teclaCtx(SDLK_DOWN); }
  quadros(60, NULL);
  snprintf(bmp, sizeof bmp, "%s-1-estilo-redonda.bmp", saida);
  quadros(1, bmp);
  // OK aplica a Redonda: a fileira de pessoas passa a desenhar o disco. O
  // primeiro OK so solta o aperto longo que abriu o modal (esperandoSoltura).
  teclaCtx(SDLK_RETURN);
  if (ctx_aberto()) teclaCtx(SDLK_RETURN);
  assert(fil_tipo(PESSOAS) == FIL_TIPO_REDONDA);
  quadros(40, NULL);

  // 2. O modal do grupo de colecao, foco na linha da Ordem, que esta no fim.
  focaFileira("Streaming");
  ctx_abrir_fileira("collection_cs", "Streaming");
  quadros(30, NULL);
  for (k = 0; k < FIL_TIPO_N; k++) teclaCtx(SDLK_DOWN);
  teclaCtx(SDLK_RIGHT);
  quadros(60, NULL);
  snprintf(bmp, sizeof bmp, "%s-2-ordem.bmp", saida);
  quadros(1, bmp);
  { SDL_Event e; memset(&e, 0, sizeof e);
    e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE; ctx_evento(&e); }
  quadros(40, NULL);

  // 3. A Home com a fileira de pessoas em Redonda, em foco.
  focaFileira("Diretores populares");
  teclaHome(SDLK_RIGHT);
  quadros(90, NULL);
  snprintf(bmp, sizeof bmp, "%s-3-home-redonda.bmp", saida);
  quadros(1, bmp);
  printf("[shot] ok\n");
  SDL_Quit();
  return 0;
}
