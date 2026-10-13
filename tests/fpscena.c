// FPS regression scenes, headless (no GL): percentiles, the p95/upload gate and
// the TV script runner. Ver src/fpscena.h. A regression must turn the gate red.
#include "../src/fpscena.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static int falhas;
static void ok(int c, const char *m) { printf("%s %s\n", c ? "ok   " : "FALHA", m); if (!c) falhas++; }

static void enche(FpsAmostra *a, double base, double lento, int nlentos, int upl) {
  int i;
  fps_zerar(a);
  for (i = 0; i < 100; i++) fps_add(a, i < nlentos ? lento : base, i == 0 ? upl : 0);
}

int main(void) {
  FpsAmostra a;
  char l[400];
  double p; long u;
  const char *base = "# cena p95 upl\nhome-scroll 1.00 4\ntitle-page 0.50 2\n";
  int i, teclas = 0, q;

  enche(&a, 1.0, 9.0, 0, 3);
  ok(fps_percentil(&a, 95) == 1.0 && fps_percentil(&a, 50) == 1.0, "percentis de quadros iguais");
  enche(&a, 1.0, 9.0, 3, 3);
  ok(fps_percentil(&a, 95) == 1.0 && fps_percentil(&a, 99) == 9.0 && fps_pior(&a) == 9.0, "3% lentos: p95 limpo, p99 e pior pegam");
  enche(&a, 1.0, 9.0, 10, 3);
  ok(fps_percentil(&a, 95) == 9.0, "10% lentos: p95 sobe");
  fps_linha(&a, "x", l, sizeof l);
  ok(strstr(l, "[fps-cena] x n=100 p50=1.00 p95=9.00") && strstr(l, "upl=3"), "linha de relatorio");

  ok(fps_base_achar(base, "title-page", &p, &u) && p == 0.5 && u == 2, "base: acha cena");
  ok(!fps_base_achar(base, "title", &p, &u) && !fps_base_achar(base, "nada", &p, &u), "base: nome exato");

  enche(&a, 1.0, 1.0, 0, 4);
  ok(fps_gate(&a, "home-scroll", 1.0, 4, 50, 0.5, l, sizeof l) && strstr(l, " OK "), "gate: igual a base passa");
  enche(&a, 1.4, 1.4, 0, 4);
  ok(fps_gate(&a, "home-scroll", 1.0, 4, 50, 0.5, l, sizeof l), "gate: ruido dentro da folga passa");
  enche(&a, 3.0, 3.0, 0, 4);
  ok(!fps_gate(&a, "home-scroll", 1.0, 4, 50, 0.5, l, sizeof l) && strstr(l, "FALHA"), "gate: p95 3x a base falha");
  enche(&a, 1.0, 1.0, 0, 40);
  ok(!fps_gate(&a, "home-scroll", 1.0, 4, 50, 0.5, l, sizeof l), "gate: uploads 10x a base falham");

  // Runner: fast frames pass the gate, slow ones turn it red.
  for (q = 0; q < 2; q++) {
    teclas = 0;
    fpscena_iniciar(NULL, base);
    for (i = 0; i < 5000 && fpscena_ativo(); i++) if (fpscena_quadro(q ? 5.0 : 0.8, 0)) teclas++;
    if (!q) ok(!fpscena_ativo() && teclas > 20 && i > 600, "runner: percorre as duas cenas e injeta teclas");
    ok(q ? fpscena_falhas() == 2 : fpscena_falhas() == 0, q ? "runner: quadros lentos reprovam as 2 cenas" : "runner: quadros rapidos passam");
  }
  fpscena_iniciar("title-page", base);
  for (i = 0; i < 5000 && fpscena_ativo(); i++) fpscena_quadro(5.0, 0);
  ok(fpscena_falhas() == 1, "runner: escolha de cena roda so uma");
  fpscena_iniciar("", base);   // an empty flag file on the TV = every scene
  for (i = 0; i < 5000 && fpscena_ativo(); i++) fpscena_quadro(5.0, 0);
  ok(fpscena_falhas() == 2, "runner: arquivo vazio roda todas as cenas");
  return falhas ? 1 : (puts("ok"), 0);
}
