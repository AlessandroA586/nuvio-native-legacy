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
  // Trakt conectado + escolha Nuvio: sem watchlist do Trakt, so Nuvio/conta.
  assert(!ajustes_fonte_traz_trakt(ajustes_fonte_resolver(AJ_SALVOS_LOCAL, 1, 1)));
  assert(ajustes_fonte_traz_conta(ajustes_fonte_resolver(AJ_SALVOS_LOCAL, 1, 1)));
  // Escolha Trakt/Simkl: conta Nuvio fora do feed; so a escolhida entra.
  assert(ajustes_fonte_traz_trakt(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 1, 1)));
  assert(!ajustes_fonte_traz_conta(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 1, 1)));
  assert(!ajustes_fonte_traz_trakt(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 1)));
  assert(!ajustes_fonte_traz_conta(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 1)));
  // So Trakt vinculado: Simkl nao e oferecido; Nuvio sempre.
  assert(!ajustes_fonte_oferecida(AJ_SALVOS_SIMKL, 1, 0));
  assert(ajustes_fonte_oferecida(AJ_SALVOS_TRAKT, 1, 0));
  assert(ajustes_fonte_oferecida(AJ_SALVOS_LOCAL, 0, 0));
  assert(!ajustes_fonte_oferecida(AJ_SALVOS_TRAKT, 0, 0));
  assert(!strcmp(ajustes_fonte_selo(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 0)), "TRAKT"));  // salvo e desvinculado

  // Os Salvos locais so pertencem a fonte LOCAL/NUVIO. Com a fonte efetiva em
  // Trakt ou Simkl eles nao podem marcar nem entrar na aba (senao um item da
  // conta Nuvio aparecia com o selo TRAKT depois de trocar a fonte).
  assert(ajustes_fonte_traz_locais(ajustes_fonte_resolver(AJ_SALVOS_LOCAL, 1, 1)));
  assert(!ajustes_fonte_traz_locais(ajustes_fonte_resolver(AJ_SALVOS_TRAKT, 1, 1)));
  assert(!ajustes_fonte_traz_locais(ajustes_fonte_resolver(AJ_SALVOS_SIMKL, 1, 1)));
  puts("bibfonte: PASS");
  return 0;
}
