// Fonte da biblioteca: a escolha (Nuvio/Trakt/Simkl) e os servicos vinculados
// decidem a fonte efetiva e o selo da aba Salvos; desvinculado cai no que resta.
#include <assert.h>
#include <string.h>
#include "../src/ajustes.h"

int main(void) {
  const char *s;
  assert(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 1, 1) == AJ_SALVOS_TRAKT);
  assert(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 1) == AJ_SALVOS_SIMKL);
  assert(ajustes_fonte_resolver(AJ_SALVOS_LOCAL, 1, 1) == AJ_SALVOS_LOCAL);
  assert(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 0) == AJ_SALVOS_TRAKT);   // Simkl saiu
  assert(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 0, 1) == AJ_SALVOS_SIMKL);   // Trakt saiu
  assert(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 0, 0) == AJ_SALVOS_LOCAL);
  assert(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 0, 0) == AJ_SALVOS_LOCAL);
  s = ajustes_fonte_selo(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 1));
  assert(!strcmp(s, "SIMKL"));
  s = ajustes_fonte_selo(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 1, 1));
  assert(!strcmp(s, "TRAKT"));
  assert(!strcmp(ajustes_fonte_selo(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 0, 0)), "LOCAL"));
  puts("bibfonte: PASS");
  return 0;
}
