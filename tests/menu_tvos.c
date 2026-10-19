// Apple TV (Dinamica) sidebar: x1.2 at UI scale <= 90%, unchanged from 100%;
// Streaming stays collapsed until focus goes down past Settings into it.
#include "../src/menu.c"
#include <assert.h>
#include <stdio.h>

static int layoutTeste = HOME_LAYOUT_DINAMICA;
int gfx_vidro_camada;
float gfx_tex_aspect_atual;
float gfx_card_forcar_cover_atual;
int ajustes_home_layout(void) { return layoutTeste; }
int ajustes_menu_explorar(void) { return 1; }
int ajustes_menu_guia(void) { return 1; }
int ajustes_menu_agenda(void) { return 1; }
int ajustes_menu_perfil(void) { return 1; }
int ajustes_rail_recolhida(void) { return 0; }
int ajustes_menu_topo(void) { return 0; }
static float escalaTeste = 1.0f;
float gfx_escala_ui(void) { return escalaTeste; }
float gfx_escala_txt(void) { return 1.0f; }
float gfx_escala_ui_base(void) { return escalaTeste; }
int home_streaming_barra(const int **p) { static const int v[3] = {4, 5, 6}; if (p) *p = v; return 3; }

static float painelReal(float ui) { escalaTeste = ui; return tvPainel().w * menuEscala(); }
static void tecla(SDL_Keycode k) {
  SDL_Event e = {0};
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k; menu_evento(&e);
}
static int visivel(void) { float ys[NV_MENU_FOCOS_TV]; tvLayout(ys, NULL); return ys[MENU_ST0] >= 0.0f; }

int main(void) {
  float base = painelReal(1.0f);
  assert(fabsf(base - 355.0f * 0.9f) < 0.01f);
  assert(fabsf(painelReal(1.1f) - base) < 0.01f);
  assert(fabsf(painelReal(0.9f) - base * 0.9f * 1.2f) < 0.01f);
  assert(fabsf(painelReal(0.8f) - base * 0.8f * 1.2f) < 0.01f);
  layoutTeste = HOME_LAYOUT_PADRAO;
  assert(fabsf(menuEscala() - 0.9f * 0.8f) < 0.001f);
  layoutTeste = HOME_LAYOUT_DINAMICA;
  escalaTeste = 1.0f;

  aberto = 1; linha = MENU_AJUSTES;
  assert(!visivel());
  tecla(SDLK_DOWN);
  assert(linha == MENU_ST0 && visivel());
  tecla(SDLK_DOWN);
  assert(linha == MENU_ST0 + 1 && visivel());
  tecla(SDLK_UP); tecla(SDLK_UP);
  assert(linha == MENU_AJUSTES && !visivel());
  tecla(SDLK_DOWN); menu_fechar();
  assert(!visivel());
  puts("PASS: Apple TV sidebar x1.2 at <=90%, streaming collapsed until below Settings");
  return 0;
}
