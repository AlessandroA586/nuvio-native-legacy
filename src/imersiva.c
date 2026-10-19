// Estado da cena do layout Imersiva. Ver imersiva.h.
#include "imersiva.h"

static float lim01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

float imv_suave(float t) { t = lim01(t); return t * t * (3.0f - 2.0f * t); }

void imv_iniciar(Imersiva *s, int fileira) {
  *s = (Imersiva){ .fileira = fileira, .anterior = -1, .dir = 1, .t = 1.0f };
}

int imv_em_troca(const Imersiva *s) { return s->t < 1.0f; }

float imv_chega(const Imersiva *s) { return s->t >= 1.0f ? 1.0f : imv_suave(s->t); }
float imv_sai(const Imersiva *s) { return s->anterior < 0 ? 0.0f : 1.0f - imv_chega(s); }

void imv_fileira(Imersiva *s, int nova) {
  if (nova == s->fileira) return;
  if (!imv_em_troca(s)) { s->anterior = s->fileira; s->t = 0.0f; }
  // Interrompida: a curva e simetrica (suave(1-t) = 1-suave(t)), entao a
  // fileira mais visivel continua saindo exatamente de onde estava. Voltar
  // para a que saia e a mesma conta: a linha do tempo anda ao contrario.
  else if (nova == s->anterior || imv_chega(s) > 0.5f) {
    s->anterior = s->fileira; s->t = 1.0f - s->t;
  }
  s->fileira = nova;
  s->dir = nova > s->anterior ? 1 : -1;
}

void imv_abrir(Imersiva *s, int coluna) { s->aberto = 1; s->coluna = coluna; }
void imv_fechar(Imersiva *s) { s->aberto = 0; }

void imv_passo(Imersiva *s, float dt, int reduzido) {
  if (reduzido) { s->t = 1.0f; s->abre = (float)s->aberto; }
  s->t = lim01(s->t + dt * (1000.0f / IMV_TROCA_MS));
  if (s->t >= 1.0f) s->anterior = -1;
  { float passo = dt * (1000.0f / IMV_ABRE_MS);
    s->abre = s->aberto ? lim01(s->abre + passo) : lim01(s->abre - passo); }
}

float imv_escada(const Imersiva *s, int k) {
  const float n = IMV_ESCADA_N - 1;
  float atraso = (k < 0 ? 0 : k > n ? n : (float)k) * IMV_ESCADA_MS;
  return imv_suave((s->t * IMV_TROCA_MS - atraso) / (IMV_TROCA_MS - n * IMV_ESCADA_MS));
}

int imv_fantasma(const Imersiva *s, int nFileiras, int lado) {
  int r = s->fileira + (lado < 0 ? -1 : 1);
  return r >= 0 && r < nFileiras ? r : -1;
}
