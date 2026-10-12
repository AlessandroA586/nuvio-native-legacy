#include "traktscrobble.h"
#include <stdio.h>
#include <string.h>

// O QUE O TRAKT ACEITA NUM PAUSE (respostas 422 dos registros 10162-10172):
// "Progress should be at least 1.0% to pause" e, a partir de 80%, "Use stop to
// scrobble". O stop a partir de 80% marcaria como assistido; este cliente so
// marca a partir de 90% (o player decide), entao entre 80 e 90 nao manda nada.
static int pausaAceita(double pct) { return pct >= 1.0 && pct < 80.0; }

int scrobble_decidir(ScrobbleEstado *s, int evento, const char *id, double pct) {
  int mesmo;
  if (!id || !*id) return SCR_NADA;
  mesmo = !strcmp(s->id, id);
  if (evento == SCR_EV_TOCANDO) {
    if (s->ativo && mesmo) return SCR_NADA;
    snprintf(s->id, sizeof s->id, "%s", id);
    s->ativo = 1;
    return SCR_START;
  }
  if (evento == SCR_EV_PAUSOU) {
    if (!s->ativo || !mesmo) return SCR_NADA;
    s->ativo = 0;
    return pausaAceita(pct) ? SCR_PAUSE : SCR_NADA;
  }
  if (evento == SCR_EV_SAIU) {
    snprintf(s->id, sizeof s->id, "%s", id);
    s->ativo = 0;
    if (pct >= 90.0) return SCR_STOP;
    return pausaAceita(pct) ? SCR_PAUSE : SCR_NADA;
  }
  return SCR_NADA;
}

void scrobble_corpo(char *dst, size_t n, const char *id, double pct) {
  char base[64];
  int t = 0, e = 0;
  char *dp;
  if (pct < 0.0) pct = 0.0;
  if (pct > 100.0) pct = 100.0;
  snprintf(base, sizeof base, "%s", id);
  dp = strchr(base, ':');
  if (dp) { sscanf(dp + 1, "%d:%d", &t, &e); *dp = 0; }
  if (t > 0 && e > 0)
    snprintf(dst, n,
             "{\"show\":{\"ids\":{\"imdb\":\"%s\"}},"
             "\"episode\":{\"season\":%d,\"number\":%d},\"progress\":%.2f}",
             base, t, e, pct);
  else
    snprintf(dst, n, "{\"movie\":{\"ids\":{\"imdb\":\"%s\"}},\"progress\":%.2f}",
             base, pct);
}

const char *scrobble_nome(int acao) {
  return acao == SCR_START ? "start" : acao == SCR_PAUSE ? "pause"
       : acao == SCR_STOP ? "stop" : "";
}
