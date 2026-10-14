#include "traktscrobble.h"
#include "metaep.h"
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

void scrobble_corpo_ep(char *dst, size_t n, const char *id, double pct,
                       const ScrobbleEpIds *ep) {
  char base[64];
  int t = 0, e = 0;
  char *dp;
  MetaEpInfo info;
  if (pct < 0.0) pct = 0.0;
  if (pct > 100.0) pct = 100.0;
  snprintf(base, sizeof base, "%s", id);
  dp = strchr(base, ':');
  if (dp) { sscanf(dp + 1, "%d:%d", &t, &e); *dp = 0; }
  memset(&info, 0, sizeof info);
  if (ep) {
    if (ep->tvdb) snprintf(info.tvdb, sizeof info.tvdb, "%s", ep->tvdb);
    if (ep->tmdb) snprintf(info.tmdb, sizeof info.tmdb, "%s", ep->tmdb);
    if (ep->trakt) snprintf(info.trakt, sizeof info.trakt, "%s", ep->trakt);
  }
  metaep_scrobble_corpo(dst, n, base, t, e, (t > 0 && e > 0) ? &info : NULL, pct);
}

void scrobble_corpo(char *dst, size_t n, const char *id, double pct) {
  scrobble_corpo_ep(dst, n, id, pct, NULL);
}

const char *scrobble_nome(int acao) {
  return acao == SCR_START ? "start" : acao == SCR_PAUSE ? "pause"
       : acao == SCR_STOP ? "stop" : "";
}
