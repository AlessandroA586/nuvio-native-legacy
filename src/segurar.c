#include "segurar.h"
#include "layout.h"

int segurar_venceu(Uint32 agora, Uint32 desde, const char *tela) {
  (void)tela;
  return desde && agora - desde >= NV_HOLD_MS;
}
