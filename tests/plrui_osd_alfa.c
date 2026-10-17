// Player OSD disc (plrui_disco_osd) with glass off must stay see-through: blend
// the recorded fill over a bright frame and check the pixel is not the opaque
// #15161A, and that it follows "Opacidade do vidro". plrui.c only, gfx stubbed.
#include "plrui.h"
#include <stdio.h>
#include <stdlib.h>

static float op = 0.78f, px[3], alfa;
int ajustes_vidro(void) { return 0; }
float ajustes_vidro_opacidade(void) { return op; }
void gfx_cor(GfxRect r, float raio, float cr, float cg, float cb, float ca) {
  (void)r; (void)raio;
  px[0] += (cr - px[0]) * ca; px[1] += (cg - px[1]) * ca; px[2] += (cb - px[2]) * ca;
  alfa = ca;
}
void gfx_anel(GfxRect r, float raio, float esp, float cr, float cg, float cb, float ca) {
  (void)r; (void)raio; (void)esp; (void)cr; (void)cg; (void)cb; (void)ca;   // rim: not the centre pixel
}

static float amostra(float opac) {
  op = opac; px[0] = .62f; px[1] = .66f; px[2] = .74f;   // bright scene
  plrui_disco_osd((GfxRect){ 100, 100, 60, 60 }, 1.0f);
  return px[1];
}

int main(void) {
  float lo = amostra(0.60f), mid = amostra(0.78f), hi = amostra(0.92f);
  printf("pixel G over bright: 60%%=%.3f 78%%=%.3f 92%%=%.3f alfa=%.2f\n", lo, mid, hi, alfa);
  if (mid < 0.20f) { puts("FAIL: disc opaque over bright frame"); return 1; }
  if (!(lo > mid && mid > hi)) { puts("FAIL: does not follow glass opacity"); return 1; }
  puts("plrui_osd_alfa: PASS");
  return 0;
}
