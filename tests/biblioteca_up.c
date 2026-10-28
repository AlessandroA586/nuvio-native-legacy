#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include "focus.h"

static Foco foco;
static int modo, temAberta, sair, pediuRelogio, acaoSel, nCelulas;
static int pickSel, tipo, ordem, fonte, buscaAberta, topMenu;
static char recado[160];
static Uint32 okDesde;
#define TECLADO_MAX 256

static int ajustes_menu_topo(void) { return topMenu; }
static int teclado_aberto(void) { return 0; }
static void teclado_evento(const SDL_Event *e) { (void)e; }
static void teclado_abrir(const char *title, const char *hint, int max) {
  (void)title; (void)hint; (void)max;
}
Uint32 SDL_GetTicks(void) { return 1; }
static int ponteiro_ok_longo(void) { return 0; }
static int celulaEmFoco(void) { return -1; }
static void menuNaCelula(int i) { (void)i; }
static void okNaCelula(int i) { (void)i; }
static void fecharAberta(void) { temAberta = 0; }
static void executarAcao(int i) { (void)i; }
static void lst_itens_mais(void) {}
static int colunas(void) { return 3; }
static int nLinhas(void) { return (nCelulas + colunas() - 1) / colunas(); }
static void descerDaFaixa(void) { foco.fileira = 2; }
static void subirParaFaixa(void) { foco.fileira = 0; }
static void pedirFonte(void) {}
static void reconstruir(void) {}
static void alternarExibicao(void) {}
#include "biblioteca_eventos.inc"

static void key(Uint32 type, SDL_Keycode code) {
  SDL_Event event = {0};
  event.type = type;
  event.key.keysym.sym = code;
  biblioteca_evento(&event);
}

int main(void) {
  for (topMenu = 0; topMenu <= 1; topMenu++) {
    for (int state = EST_TITULOS; state <= EST_ITENS; state++) {
      modo = state == EST_TITULOS ? MODO_SALVOS : MODO_LISTAS;
      temAberta = state == EST_ITENS;
      for (int row = 0; row < (temAberta ? 1 : 2); row++) {
        for (int col = 0; col < 3; col++) {
          foco.fileira = row;
          foco.coluna = acaoSel = pickSel = col;
          sair = pediuRelogio = 0;
          key(SDL_KEYUP, SDLK_UP);
          assert(!biblioteca_quer_sair() && !biblioteca_pediu_relogio());
          key(SDL_KEYDOWN, SDLK_UP);
          if (biblioteca_quer_sair() != topMenu || biblioteca_pediu_relogio() != !topMenu) {
            fprintf(stderr, "FAIL: Up state=%d row=%d col=%d topMenu=%d\n", state, row, col, topMenu);
            return 1;
          }
          assert(!biblioteca_pediu_relogio()); // Requests are consumed once.
          assert(foco.fileira == row && foco.coluna == col);
          assert(acaoSel == col && pickSel == col);
        }
      }
      // First Up from content stays in Library; next Up reaches navigation.
      sair = pediuRelogio = 0;
      foco.fileira = temAberta ? 1 : BIB_FIL_GRADE;
      foco.coluna = 1;
      key(SDL_KEYDOWN, SDLK_UP);
      assert(!biblioteca_quer_sair() && !biblioteca_pediu_relogio());
      assert(foco.fileira == 0);
      key(SDL_KEYDOWN, SDLK_UP);
      assert(biblioteca_quer_sair() == topMenu);
      assert(biblioteca_pediu_relogio() == !topMenu);
    }
  }
  // Integration enters open lists directly, retaining the selected action.
  modo = MODO_LISTAS;
  temAberta = 1;
  foco.fileira = 0;
  foco.coluna = acaoSel = 2;
  nCelulas = 0;
  key(SDL_KEYDOWN, SDLK_DOWN);
  assert(foco.fileira == 0 && foco.coluna == 2);
  nCelulas = 7;
  key(SDL_KEYDOWN, SDLK_DOWN);
  assert(foco.fileira == 1 && foco.coluna == 0 && acaoSel == 2);
  key(SDL_KEYDOWN, SDLK_UP);
  assert(foco.fileira == 0 && foco.coluna == 2);
  puts("Library Up: PASS (tabs, selectors, list actions; top menu and sidebar)");
  return 0;
}
