// #461: menu na barra do topo. A rail some (conteudo usa a largura toda) e as
// setas giram: ESQUERDA/DIREITA andam pelos itens, BAIXO devolve o foco ao
// conteudo, CIMA nao faz nada, OK escolhe, Voltar fecha. Desligado, a rail volta.
#include "ajustes.h"
#include "layout.h"
#include "menu.h"
#include "biblioteca.h"
#include "ajustes.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  SDL_memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN;
  e.key.keysym.sym = k;
  menu_evento(&e);
}
static void assentar(void) { for (int i = 0; i < 200; i++) menu_atualizar(0.016f, 0); }
static void abrir(int destino) {
  menu_fechar(); assentar();
  menu_escolheu(); menu_mudou_destino();
  menu_definir_destino(destino);
  menu_abrir();
}

static char dir[] = "/tmp/nuvio-menu-topo-XXXXXX", caminho[700];
static void gravar(const char *txt) {
  FILE *f = fopen(caminho, "w"); assert(f);
  fputs(txt, f); fclose(f);
  ajustes_dir(dir);
}

int main(void) {
  assert(mkdtemp(dir));
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);

  // Padrao: a rail de sempre.
  gravar("homeLayoutLocal 0\nmodernSidebar 0\n");
  assert(!ajustes_menu_topo() && ajustes_rail_largura_fixa() > 0.0f);

  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\n");
  assert(ajustes_menu_topo());
  assert(ajustes_rail_largura_fixa() == 0.0f);
  assert(ajustes_conteudo_x() == NV_CONTENT_PAD);
  menu_iniciar();
  assert(menu_barra_borda() == 0.0f);

  abrir(MENU_INICIO);
  tecla(SDLK_RIGHT); tecla(SDLK_UP);          // anda; cima nao fecha nem escolhe
  assert(menu_aberto() && !menu_escolheu());
  tecla(SDLK_RIGHT); tecla(SDLK_LEFT); tecla(SDLK_RETURN);
  assert(menu_escolheu() && menu_destino() == MENU_EXPLORAR);

  abrir(MENU_INICIO);
  tecla(SDLK_RIGHT); tecla(SDLK_DOWN);        // baixo volta ao conteudo
  assert(!menu_aberto() && !menu_escolheu() && menu_destino() == MENU_INICIO);

  abrir(MENU_INICIO);
  tecla(SDLK_LEFT); tecla(SDLK_RETURN);       // sem volta na ponta esquerda
  assert(menu_destino() == MENU_INICIO);

  abrir(MENU_INICIO);
  tecla(SDLK_ESCAPE);
  assert(!menu_aberto() && !menu_escolheu());

  // No Dinamica a opcao nao vale: a pilula dele ja e o topo.
  gravar("homeLayoutLocal 2\nmodernSidebar 0\nmenuTopoLocal 1\n");
  assert(!ajustes_menu_topo());

  // Desligado de novo: a rail e as setas dela voltam.
  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 0\n");
  assert(!ajustes_menu_topo() && ajustes_rail_largura_fixa() > 0.0f);
  abrir(MENU_INICIO);
  tecla(SDLK_DOWN); tecla(SDLK_RIGHT);
  assert(menu_escolheu() && menu_destino() == MENU_EXPLORAR);

  // #461: com a barra no topo, CIMA no primeiro item da tela abre a barra.
  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 1\n");
  menu_fechar(); assentar();
  assert(ajustes_menu_topo());
  biblioteca_iniciar();
  { SDL_Event e; SDL_memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_UP;
    biblioteca_evento(&e);
    assert(biblioteca_quer_sair()); }
  menu_fechar(); assentar();
  ajustes_iniciar();
  { SDL_Event e; int i; SDL_memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_UP;
    for (i = 0; i < 8 && !ajustes_quer_sair(); i++) ajustes_evento(&e);
    assert(ajustes_quer_sair()); }
  // Sem a barra no topo, CIMA na Biblioteca nao pede saida.
  gravar("homeLayoutLocal 0\nmodernSidebar 0\nmenuTopoLocal 0\n");
  biblioteca_iniciar();
  { SDL_Event e; SDL_memset(&e, 0, sizeof e); e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_UP;
    biblioteca_evento(&e);
    assert(!biblioteca_quer_sair()); }

  unlink(caminho);
  { char t[700]; snprintf(t, sizeof t, "%s/ajustes.tmp", dir); unlink(t);
    snprintf(t, sizeof t, "%s/envio-151.txt", dir); unlink(t); }
  rmdir(dir);
  puts("menu_topo: ok");
  return 0;
}
