// Search result rows drag-scroll only in Mobile mode; TV keeps drag-as-arrows.
#include "../src/busca.c"
#include <assert.h>

static void arrasto(int mobile) {
  PonteiroRolagem e = { PONT_ROL_INICIO, 0, 0.0f, 0.0f, 0.0f, 0.0f };
  layout_modo_definir(mobile);
  nFil = 1; fil[0].n = 20; fil[0].pessoas = 0; fil[0].melhor = 0;
  scrollX[0] = 0.0f; toqueLivreX[0] = 0; toqueFileira = -1;
  e.x = BU_RES_X + 10.0f; e.y = BU_RES_Y + 10.0f;
  int captura = toqueBuscaRolar(&e);
  e.fase = PONT_ROL_MOVER; e.delta = -200.0f;
  toqueBuscaRolar(&e);
  if (mobile) assert(captura == 1 && scrollX[0] == 200.0f && toqueLivreX[0]);
  else assert(captura == 0 && scrollX[0] == 0.0f && !toqueLivreX[0]);
}

int main(void) {
  layout_tela_definir(1920, 1080);
  arrasto(0);
  arrasto(1);
  puts("PASS: Busca row drag is Mobile only");
  return 0;
}
