// Action row: gap after a focused button stays NV_DETW2_BTN_GAP, and the
// "Assistir do começo" pill opens slowly and smoothly. Pure.
#include "detail_acoes.h"
#include <stdio.h>
#include <math.h>

#define BTN_GAP 18.0f
#define FOCO_SX 1.114f
#define FOCO_SY 1.147f
static int falhas;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %d: %s\n", __LINE__, #c); falhas++; } } while (0)

static void gap(void) {
  for (int k = 0; k < 8; k++) {
    float s = 0.5f + 0.25f * k;                  // UI scales
    float w = 321.0f * s, x = 80.0f * s, g = BTN_GAP * s;
    for (int foc = 0; foc <= 1; foc++) {
      float prox = x + det_avanco(w, foc, FOCO_SX, g);
      float dir = x + w + (foc ? det_foco_extra(w, FOCO_SX) : 0.0f);   // drawn right edge
      CHECK(fabsf((prox - dir) - g) < 0.001f);
      if (foc) CHECK((x + w + g) - dir < g - 1.0f);   // old advance w+gap left < g
    }
  }
}

static void inicio(int dtms) {
  float larg = 0, rot = 0, prev = 0, maxPasso = 0, t = 0, tFim = -1;
  float dt = dtms * 0.001f;
  anim_politica_reduzida = 0;
  for (int i = 0; i < 400; i++, t += dt) {
    anim_expande(&larg, &rot, 1.0f, dt, NV_DET_INICIO_LARG_MS, NV_DET_INICIO_ROT_MS);
    float w = det_inicio_largura(larg);
    CHECK(w >= prev - 1e-6f);                   // monotonic
    if (w - prev > maxPasso) maxPasso = w - prev;
    prev = w;
    if (tFim < 0 && larg >= 1.0f && rot >= 1.0f) tFim = t + dt;
  }
  CHECK(prev == 1.0f);
  CHECK(tFim * 1000.0f >= 300.0f);              // slower than the old 280 ms
  CHECK(maxPasso <= 3.2f * dtms / NV_DET_INICIO_LARG_MS);   // no snap: cubic-out peak slope is 3/T
  // frame-rate independent: width reaches 1 at the same time (within a frame)
  printf("dt=%d fim=%.0fms passo=%.3f\n", dtms, tFim * 1000.0f, maxPasso);
}

int main(void) {
  gap();
  inicio(16); inicio(33); inicio(8);
  if (falhas) { printf("%d falhas\n", falhas); return 1; }
  puts("OK");
  return 0;
}
