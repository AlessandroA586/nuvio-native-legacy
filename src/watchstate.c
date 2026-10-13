#include "watchstate.h"
#include "js.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Valor da chave `chave` de profundidade 1 em [ini,fim): o resource e o root
// repetem "types"/"events" aninhados, e js_texto pegaria o primeiro.
static const char *raiz(const char *ini, const char *fim, const char *chave) {
  size_t kl = strlen(chave);
  int prof = 0;
  const char *p;
  if (!ini) return NULL;
  if (!fim) fim = ini + strlen(ini);
  for (p = ini; p < fim; p++) {
    if (*p == '"') {
      const char *a = p + 1, *q = a;
      while (q < fim && *q != '"') { if (*q == '\\' && q + 1 < fim) q++; q++; }
      if (prof == 1 && (size_t)(q - a) == kl && !strncmp(a, chave, kl)) {
        const char *v = q + 1;
        while (v < fim && isspace((unsigned char)*v)) v++;
        if (v < fim && *v == ':') {
          for (v++; v < fim && isspace((unsigned char)*v); v++) ;
          return v < fim ? v : NULL;
        }
      }
      p = q;
    } else if (*p == '{' || *p == '[') prof++;
    else if (*p == '}' || *p == ']') prof--;
  }
  return NULL;
}

// Strings de um array JSON (v no '['), cada uma entregue a `cb`.
static void strings(const char *v, void (*cb)(const char *, void *), void *u) {
  const char *fim, *p;
  if (!v || *v != '[' || !(fim = js_fim(v))) return;
  for (p = v + 1; p < fim; p++) {
    if (*p == '"') {
      char s[WS_PREF_TAM];
      size_t k = 0;
      for (p++; p < fim && *p != '"'; p++) {
        if (*p == '\\' && p + 1 < fim) p++;
        if (k + 1 < sizeof s) s[k++] = *p;
      }
      s[k] = 0;
      if (k) cb(s, u);
    }
  }
}

static void cbTipo(const char *s, void *u) {
  WsDecl *d = u;
  size_t o = strlen(d->tipos), l = strlen(s), i;
  if (!o) { d->tipos[0] = '|'; o = 1; }
  if (o + l + 2 > sizeof d->tipos) return;
  for (i = 0; i < l; i++) d->tipos[o + i] = (char)tolower((unsigned char)s[i]);
  d->tipos[o + l] = '|'; d->tipos[o + l + 1] = 0;
}
static void cbPref(const char *s, void *u) {
  WsDecl *d = u;
  if (d->nPref < WS_PREF_MAX) snprintf(d->pref[d->nPref++], WS_PREF_TAM, "%s", s);
}
static void cbEvento(const char *s, void *u) {
  static const struct { const char *n; int bit; } ev[] = {
    {"start", WS_START}, {"pause", WS_PAUSE}, {"stop", WS_STOP},
    {"played", WS_PLAYED}, {"unplayed", WS_UNPLAYED}};
  size_t i;
  for (i = 0; i < sizeof ev / sizeof *ev; i++)
    if (!strcmp(s, ev[i].n)) *(int *)u |= ev[i].bit;   // outros nomes: ignorados
}

void ws_decl_ler(const char *corpo, WsDecl *d) {
  const char *res, *fimR, *e, *ws, *fimW, *push, *fimP, *ver, *evs;
  int achou = 0;
  memset(d, 0, sizeof *d);
  if (!corpo) return;
  res = raiz(corpo, NULL, "resources");
  if (!res || *res != '[' || !(fimR = js_fim(res))) return;
  for (e = res + 1; e < fimR && !achou; ) {
    while (e < fimR && (isspace((unsigned char)*e) || *e == ',')) e++;
    if (e >= fimR) break;
    if (*e == '{') {
      const char *fe = js_fim(e);
      char nome[24] = "";
      if (!fe) break;
      js_texto_raiz_em(e, fe, "name", nome, sizeof nome);
      if (!strcmp(nome, "watch_state")) {
        const char *tp = raiz(e, fe, "types"), *pf = raiz(e, fe, "idPrefixes");
        achou = 1;
        // O resource manda; na falta, valem os da raiz do manifesto.
        strings(tp ? tp : raiz(corpo, NULL, "types"), cbTipo, d);
        strings(pf ? pf : raiz(corpo, NULL, "idPrefixes"), cbPref, d);
      }
      e = fe + 1;
    } else if (*e == '"') {   // forma curta "watch_state": herda os da raiz
      const char *a = e + 1;
      for (e++; e < fimR && *e != '"'; e++) if (*e == '\\') e++;
      if (!strncmp(a, "watch_state\"", 12)) {
        achou = 1;
        strings(raiz(corpo, NULL, "types"), cbTipo, d);
        strings(raiz(corpo, NULL, "idPrefixes"), cbPref, d);
      }
      e++;
    } else e++;
  }
  if (!achou) return;
  // O root decide a versao e as metades. Sem watchState{push{}} o addon nao
  // pediu push; versao != 2 e o contrato v1/futuro, que nao misturamos.
  ws = raiz(corpo, NULL, "watchState");
  if (!ws || *ws != '{' || !(fimW = js_fim(ws))) return;
  ver = raiz(ws, fimW, "version");
  if (!ver || atoi(ver) != 2) return;
  push = raiz(ws, fimW, "push");
  if (!push || *push != '{' || !(fimP = js_fim(push))) return;
  evs = raiz(push, fimP, "events");
  if (evs && *evs == '[') strings(evs, cbEvento, &d->eventos);
  else d->eventos = WS_TODOS;     // omitido = todo evento de reproducao
  d->eventos &= WS_TODOS;
  { const char *vw = raiz(ws, fimW, "viewers");
    d->viewers = vw && !strncmp(vw, "true", 4); }
  d->push = d->eventos != 0;
}

int ws_decl_aceita(const WsDecl *d, int ev, const char *tipo,
                   const char *metaId, const char *videoId) {
  int k;
  if (!d || !d->push || !(d->eventos & ev)) return 0;
  // viewers:true pede ?viewer=<slug> por pessoa (W8, ainda sem vinculo). Sem
  // ele o evento iria para o DONO da configuracao: melhor nao enviar.
  if (d->viewers) return 0;
  if (d->tipos[0]) {
    char marca[40];
    if (!tipo || !tipo[0]) return 0;
    snprintf(marca, sizeof marca, "|%s|", tipo);
    for (k = 0; marca[k]; k++) marca[k] = (char)tolower((unsigned char)marca[k]);
    if (!strstr(d->tipos, marca)) return 0;
  }
  if (d->nPref > 0) {
    for (k = 0; k < d->nPref; k++) {
      size_t l = strlen(d->pref[k]);
      if (metaId && !strncmp(metaId, d->pref[k], l)) return 1;
      if (videoId && !strncmp(videoId, d->pref[k], l)) return 1;
    }
    return 0;
  }
  return 1;
}
