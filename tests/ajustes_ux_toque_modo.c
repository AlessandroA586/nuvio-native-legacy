// Inline pointer editing (Theme row) is Mobile only; TV keeps the keyboard editor.
#include "../src/ajustes.c"
#include <assert.h>

int main(void) {
  Aj2Pose p = {0};
  p.lA = 1.0f; p.lS = 1.0f;
  uxIndice = 2; uxEditor = 1; uxOp = AJ_TEMA; uxRestaurar = 0; uxAvisoRisco = 0;
  assert(ajEditandoLinha(AJ_TEMA));
  layout_modo_definir(0);
  assert(!ajInlineAtivo(0, &p));
  layout_modo_definir(1);
  assert(ajInlineAtivo(0, &p));
  assert(!ajInlineAtivo(1, &p));   // a modal still wins in Mobile
  puts("PASS: inline editor is Mobile only");
  return 0;
}
