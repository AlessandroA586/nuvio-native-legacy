// Mobile "Lista": the page header must start below the badge/clock band.
#include "../src/ajustes.c"
#include <assert.h>

int main(void) {
  float escalas[2] = { 0.85f, 1.0f };
  int i;
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  layout_modo_definir(1);
  valor[AJ_LAYOUT_AJUSTES] = 1; assert(ajustes_layout_lista());
  for (i = 0; i < 2; i++) {
    valor[AJ_TAMANHO_AJUSTES] = escalas[i] < 0.9f ? 0 : 1;
    assert(aj2PaginaR().y >= 96.0f);                 // clock/badge pills end near y = 93
    assert(aj2ListaR().y >= aj2PaginaR().y + aj2PaginaR().h);
  }
  puts("PASS: Lista header below badge/clock");
  return 0;
}
