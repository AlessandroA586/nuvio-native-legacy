// Confirmacao de busca: o destino otimista da barra nao prova progresso real.
#ifndef NV_CREDITOSAUTO_H
#define NV_CREDITOSAUTO_H
#include <math.h>
#include <string.h>

typedef struct {
  int buscaPendente, buscaRecuo, fonteMudou;
  double fimBusca, inicioBusca;
  char fonte[4096];
} CreditosAuto;

static inline void creditosauto_zerar(CreditosAuto *s) { memset(s, 0, sizeof *s); }

// Marcadores de rede podem pertencer a outro corte apos trocar a fonte.
static inline void creditosauto_fonte(CreditosAuto *s, const char *url) {
  size_t n;
  if (!url || !*url) return;
  n = strlen(url);
  if (n >= sizeof s->fonte) { s->fonteMudou = 1; s->buscaPendente = 0; }
  else if (!s->fonte[0]) memcpy(s->fonte, url, n + 1);
  else if (strcmp(s->fonte, url)) { s->fonteMudou = 1; s->buscaPendente = 0; }
}

static inline void creditosauto_buscar(CreditosAuto *s, double destino) {
  s->fimBusca = destino;
  s->buscaPendente = 1; s->buscaRecuo = 0;
}

static inline void creditosauto_recuar(CreditosAuto *s, double de, double para) {
  if (!isfinite(de) || !isfinite(para) || para >= de) return;
  s->fimBusca = para; s->inicioBusca = de;
  s->buscaPendente = s->buscaRecuo = 1;
}

// No recuo, tempo/fim antigos nao confirmam a busca. Next so volta a contar
// apos progresso no destino; no avanco, o fim nativo tambem confirma.
static inline int creditosauto_aguardar(CreditosAuto *s, double pos, int tocando, int terminou) {
  if (!s->buscaPendente) return 0;
  if ((!s->buscaRecuo && terminou) ||
      (tocando && isfinite(pos) && pos > s->fimBusca + 0.25 &&
       (!s->buscaRecuo || pos < s->inicioBusca))) s->buscaPendente = 0;
  return s->buscaPendente;
}
#endif
