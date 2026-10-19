// Exercise the actual Saved-tab filtering and counters.
void bibfonte_library_shot(void);
#include "../src/biblioteca.c"
#include <assert.h>
int bibfonte_library_count(void) {
  modo = MODO_SALVOS; tipo = TIPO_TODOS; ordem = ORD_ADICIONADOS;
  reconstruir();
  assert(contaModo[MODO_SALVOS] == nFiltro);
  return nFiltro;
}
void bibfonte_library_shot(void) {
  biblioteca_atualizar(1.0f / 60, SDL_GetTicks());
  biblioteca_desenhar(SDL_GetTicks());
}
