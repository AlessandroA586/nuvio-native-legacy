// Action row: drawn gap between neighbours stays 18 whichever button is
// focused. Row laid out the way heroWeb does (helpers), edges drawn by the
// scale-about-centre rule. Pure.
#include "detail_acoes.h"
#include <stdio.h>
#include <math.h>

#define GAP 18.0f
#define SX 1.114f
#define SY 1.147f
static int falhas;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %d: %s\n", __LINE__, #c); falhas++; } } while (0)

static void linha(const float *w, const float *s, int circ0, int n, int f, float u) {
  float bx = 80.0f * u, prevR = 0;
  for (int i = 0; i < n; i++) {
    int foc = i == f, circ = i >= circ0;   // circles also shift right by their growth
    float x = bx + (circ && foc ? det_foco_extra(w[i] * u, s[i]) : 0.0f);
    float cx = x + w[i] * u * 0.5f, dw = w[i] * u * (foc ? s[i] : 1.0f);
    if (i > 0 && circ) CHECK(fabsf((cx - dw * 0.5f - prevR) - GAP * u) < 0.01f);
    prevR = cx + dw * 0.5f;
    bx = circ ? x + w[i] * u + (foc ? det_foco_extra(w[i] * u, s[i]) : 0.0f) + GAP * u
              : x + det_avanco(w[i] * u, foc, s[i], GAP * u);
  }
}

int main(void) {
  float w3[] = {321, 72, 72}, s3[] = {SX, SY, SY};
  float wr[] = {321, 200, 72}, sr[] = {SX, SY, SY};
  for (int k = 0; k < 8; k++) {
    float u = 0.5f + 0.25f * k;
    for (int f = -1; f < 3; f++) { linha(w3, s3, 1, 3, f, u); linha(wr, sr, 2, 3, f, u); }
  }
  if (falhas) { printf("%d falhas\n", falhas); return 1; }
  puts("OK");
  return 0;
}
