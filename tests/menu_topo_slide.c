// Barra do topo: a capsula de selecao desliza entre os itens em ~180 ms
// (por tempo, ease-out) e salta com "Animacoes reduzidas".
#include "ajustes.h"
#include "menu.h"
#include "dados.h"
#include "nvtmp.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  SDL_memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  menu_evento(&e);
}
static void passo(float s) { menu_atualizar(s, 0); }

static char dir[NVT_MAX], caminho[700];
static void gravar(const char *txt) {
  FILE *f = fopen(caminho, "w"); assert(f);
  fputs(txt, f); fclose(f);
  ajustes_dir(dir);
}

int main(void) {
  float p;
  int i;
  nvt_set(dir, sizeof dir, "/nuvio-menu-slide-XXXXXX");
  assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);

  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\nanimacoes 0\n");
  assert(ajustes_menu_topo() && !ajustes_animacoes_reduzidas());
  menu_iniciar();
  menu_abrir();
  for (i = 0; i < 60; i++) passo(0.016f);
  assert(menu_topo_pos() == 0.0f);

  // Move o foco: a capsula nao salta, passa pelo meio e chega em ~180 ms.
  tecla(SDLK_RIGHT);
  passo(0.016f);
  p = menu_topo_pos();
  assert(p > 0.0f && p < 1.0f);
  for (i = 0; i < 4; i++) passo(0.016f);       // 80 ms
  p = menu_topo_pos();
  assert(p > 0.3f && p < 1.0f);
  for (i = 0; i < 8; i++) passo(0.016f);       // 208 ms
  assert(menu_topo_pos() == 1.0f);

  // Mesmo tempo de parede em passos de 33 ms: o mesmo destino.
  tecla(SDLK_RIGHT);
  for (i = 0; i < 6; i++) passo(0.033f);       // ~200 ms
  assert(menu_topo_pos() == 2.0f);

  // Animacoes reduzidas: salta no primeiro quadro.
  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\nanimacoes 1\n");
  assert(ajustes_animacoes_reduzidas());
  tecla(SDLK_RIGHT);
  passo(0.016f);
  assert(menu_topo_pos() == 3.0f);

  unlink(caminho);
  { char t[700]; snprintf(t, sizeof t, "%s/ajustes.tmp", dir); unlink(t); }
  rmdir(dir);
  puts("menu_topo_slide: ok");
  return 0;
}
