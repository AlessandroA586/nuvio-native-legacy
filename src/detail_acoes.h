// Action row (detail.c): spacing around a focused button and the timing of the
// "Assistir do começo" pill. Pure, so tests/detail_acoes_gap.c can sample it.
#ifndef NV_DETAIL_ACOES_H
#define NV_DETAIL_ACOES_H
#include "anim.h"

// Focused buttons grow around their centre (desenhaBotao, scale `s`). The
// layout must advance by the growth on the right too, or the gap to the next
// button shrinks by it (Play pill: 18 -> ~0).
static inline float det_foco_extra(float w, float s) { return w * (s - 1.0f) * 0.5f; }

// Distance from a button's layout x to the next button's x.
static inline float det_avanco(float w, int focado, float s, float gap) {
  return w + (focado ? det_foco_extra(w, s) : 0.0f) + gap;
}

// "Assistir do começo" pill: width opens first, then the label (anim_expande).
// Cubic ease-out, no bounce, dt based; slower than the old 160/120 ms.
#define NV_DET_INICIO_LARG_MS 360.0f
#define NV_DET_INICIO_ROT_MS  220.0f
static inline float det_inicio_largura(float t) { return anim_saida(t); }
#endif
