#include "bufavanco.h"
#include <stdio.h>
static int falhas;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); falhas++; } } while (0)
int main(void) {
  BufAvanco s = {0};
  // Fonte A trava em 12.0 s; primeira checagem arma a janela e segura.
  CHECK(bufavanco_segura(&s, 100, 31000, 31000, 12.0) == 1);
  CHECK(bufavanco_segura(&s, 100, 36000, 36000, 12.0) == 1);
  CHECK(bufavanco_segura(&s, 100, 41000, 41000, 12.0) == 0);   // sem avanco: troca
  // Fonte B (chave nova) ja a 31 s de buffering, borda em 12.3: nao herda o
  // estado de A, ganha janela propria em vez de ser trocada de imediato.
  CHECK(bufavanco_segura(&s, 200, 31000, 42000, 12.3) == 1);
  CHECK(bufavanco_segura(&s, 200, 36000, 47000, 12.3) == 1);
  CHECK(bufavanco_segura(&s, 200, 41000, 52000, 12.3) == 0);
  // Borda andando >= 0.5 s por janela segura; reinicia a janela.
  CHECK(bufavanco_segura(&s, 300, 31000, 60000, 5.0) == 1);
  CHECK(bufavanco_segura(&s, 300, 38000, 67000, 6.0) == 1);
  CHECK(bufavanco_segura(&s, 300, 45000, 74000, 6.0) == 1);
  CHECK(bufavanco_segura(&s, 300, 48500, 77500, 6.0) == 0);
  // Fonte que so cutuca 0.5 s a cada 30 s nao segura para sempre.
  { BufAvanco p = {0}; unsigned t = 0; double f = 1.0; int liberou = 0, i;
    for (i = 0; i < 40; i++) { t += 5000; if (i % 6 == 5) f += 0.6;
      if (!bufavanco_segura(&p, 1, 30000 + t, t, f)) liberou = 1; }
    CHECK(liberou); }
  // Novo episodio de buffering (ms recomeca) na mesma fonte rearma.
  CHECK(bufavanco_segura(&s, 300, 31000, 90000, 6.0) == 1);
  if (falhas) return 1;
  fprintf(stderr, "bufavanco: PASS\n");
  return 0;
}
