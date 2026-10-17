// Search and menu anchors use a fixed 100% reference, never scaled expectations.
#include "../src/busca.c"
#include <math.h>
#include <stdio.h>

int test_search_anchors(float scale) {
  float x, y, w, h;
  gfx_escala_txt_definir(1.0f);
  float old = gfx_escala_txt_entrar();
  int pill = menu_pilula_rect(&x, &y, &w, &h);
  float refX = x, refY = y, refH = h;
  float expected = buTopoEsq() * gfx_escala();
  gfx_escala_sair(old);
  gfx_escala_txt_definir(scale);
  old = gfx_escala_txt_entrar();
  menu_pilula_rect(&x, &y, &w, &h);
  float field = buTopoEsq() * gfx_escala();
  float results = buResY() * gfx_escala();
  int bad = fabsf(field - expected) > 2.0f || fabsf(results - 64.0f) > 2.0f;
  if (pill) bad |= fabsf(x - refX) > 2.0f || fabsf(y - refY) > 2.0f ||
                   fabsf(h - refH) > 2.0f || field < y + h + 18.0f;
  printf("%s Search layout %d %.0f%%: field %.1f (100%% %.1f), results %.1f (expected 64), pill %.1f,%.1f h %.1f\n",
         bad ? "FAIL" : "PASS", ajustes_home_layout(), scale * 100, field, expected, results, x, y, h);
  gfx_escala_sair(old);
  return bad;
}
