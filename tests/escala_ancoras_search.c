// Search field/results keep their screen anchors as text scale changes.
#include "../src/busca.c"
#include <math.h>
#include <stdio.h>

int test_search_anchors(float scale) {
  float old = gfx_escala_txt_entrar();
  float x, y, w, h;
  float expected = menu_pilula_rect(&x, &y, &w, &h)
                 ? fmaxf(64.0f, y + h + 20.0f) : 64.0f;
  float field = buTopoEsq() * gfx_escala();
  float results = buResY() * gfx_escala();
  int bad = fabsf(field - expected) > 2.0f || fabsf(results - 64.0f) > 2.0f;
  printf("%s Search layout %d %.0f%%: field %.1f (expected %.1f), results %.1f (expected 64)\n",
         bad ? "FAIL" : "PASS", ajustes_home_layout(), scale * 100, field, expected, results);
  gfx_escala_sair(old);
  return bad;
}
