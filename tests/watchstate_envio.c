// watch_state v2 push, passo 3: o fio de entrega e o filtro por addon, com
// rede, disco, addons e perfil falsos. O relogio e o real, entao o backoff em
// si (30 s .. 6 h) fica em tests/watchstate_fila.c; aqui vale o que o fio faz.
//
//   bash tests/watchstate_envio.sh
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "addons.h"
#include "dados.h"
#include "perfis.h"
#include "rede.h"
#include "watchstate_envio.h"

static int falhas;
#define OK(c) do { if (!(c)) { printf("FALHOU %d: %s\n", __LINE__, #c); falhas++; } } while (0)

// ---- Rede falsa ------------------------------------------------------------
static pthread_mutex_t tr = PTHREAD_MUTEX_INITIALIZER;
static unsigned caps;
static int nA, nC, nD, nB, prazoOk = 1, postOk = 1, jsonOk = 1;
static char corpoA[1024], urlA[512];

unsigned rede_pedido_capacidades(void) { return caps; }
int rede_pedir(const RedePedido *p, RedeResposta *r) {
  int st = 204;
  memset(r, 0, sizeof *r);
  pthread_mutex_lock(&tr);
  if (!p->metodo || strcmp(p->metodo, "POST")) postOk = 0;
  if (p->prazo_ms != 15000) prazoOk = 0;
  if (!p->cabecalhos || !p->cabecalhos[0] || !strstr(p->cabecalhos[0], "application/json")) jsonOk = 0;
  if (strstr(p->url, "a.test")) { nA++; snprintf(urlA, sizeof urlA, "%s", p->url);
                                  snprintf(corpoA, sizeof corpoA, "%.*s", (int)p->n_corpo, (const char *)p->corpo); }
  else if (strstr(p->url, "b.test")) nB++;
  else if (strstr(p->url, "c.test")) { nC++; st = 401; }
  else if (strstr(p->url, "d.test")) { nD++; st = 503; }
  pthread_mutex_unlock(&tr);
  if (st == 204 && strstr(p->url, "a.test")) usleep(300000);   // addon lento
  r->erro = REDE_OK; r->status = st;
  return 0;
}
void rede_resposta_limpar(RedeResposta *r) { (void)r; }

// ---- Disco, perfil e addons falsos ------------------------------------------
static char *disco;
int dados_gravar(const char *n, const char *c) { (void)n; pthread_mutex_lock(&tr); free(disco); disco = strdup(c); pthread_mutex_unlock(&tr); return 1; }
char *dados_ler(const char *n) { (void)n; return NULL; }
int dados_apagar(const char *n) { (void)n; pthread_mutex_lock(&tr); free(disco); disco = NULL; pthread_mutex_unlock(&tr); return 1; }
static int perfil = 1, perfilAddons = 1;
int perfis_ativo(void) { return perfil; }
int perfis_ativo_addons(void) { return perfilAddons; }

static const char *bases[] = {"http://a.test/x", "http://b.test/x", "http://c.test/x", "http://d.test/x"};
int addons_n(void) { return 4; }
const char *addons_base(int i) { return bases[i]; }
int addons_watchstate(int i, WsDecl *d) {
  memset(d, 0, sizeof *d);
  if (i == 1) return 0;                       // b.test nao declara watch_state
  d->push = 1; d->eventos = WS_TODOS;
  return 1;
}

static WsEvento filme(int ev, long long pos) {
  WsEvento e;
  memset(&e, 0, sizeof e);
  e.ev = ev; strcpy(e.tipo, "movie");
  strcpy(e.metaId, "tt0133093"); strcpy(e.videoId, "tt0133093");
  e.atMs = 1757441718000LL; e.iniMs = 1757438900000LL; e.posMs = pos; e.durMs = 8160000;
  e.season = e.episode = -1; strcpy(e.imdb, "tt0133093");
  return e;
}

static void espera(int *n, int quer) {
  int i;
  for (i = 0; i < 200 && *n < quer; i++) usleep(10000);
}
static long long agoraMs(void) {
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

int main(void) {
  WsEvento e;
  long long t0, dt;
  int a, c, d;
  alarm(8);   // start -> pause -> stop deve terminar mesmo com 401/503

  // VIDAA (so GET): nada sai, nem o fio nasce
  caps = 0; e = filme(WS_START, 1000);
  wsenvio_emitir(&e);
  usleep(200000);
  OK(nA + nB + nC + nD == 0);

  // perfil que usa os addons de outro: nada sai
  caps = REDE_CAP_CORPO; perfil = 2; perfilAddons = 1;
  wsenvio_emitir(&e);
  usleep(200000);
  OK(nA + nB + nC + nD == 0);
  perfil = perfilAddons = 1;

  // evento invalido (sem videoId): nada sai
  e.videoId[0] = 0; wsenvio_emitir(&e); usleep(200000);
  OK(nA + nB + nC + nD == 0);

  // emitir nunca espera pela rede (a.test demora 300 ms por pedido)
  e = filme(WS_START, 1000);
  t0 = agoraMs(); wsenvio_emitir(&e); dt = agoraMs() - t0;
  OK(dt < 100);

  // corpo e URL certos so para quem declara; 401 e 503 tentam uma vez so
  espera(&nA, 1); espera(&nC, 1); espera(&nD, 1);
  usleep(600000);
  pthread_mutex_lock(&tr); a = nA; c = nC; d = nD;
  OK(nA == 1 && nB == 0 && nC == 1 && nD == 1);
  OK(!strcmp(urlA, "http://a.test/x/watch_state/push/movie/tt0133093.json"));
  OK(strstr(corpoA, "\"event\":\"start\"") && strstr(corpoA, "\"videoId\":\"tt0133093\"") &&
     strstr(corpoA, "\"positionMs\":1000") && strstr(corpoA, "\"durationMs\":8160000") &&
     strstr(corpoA, "\"scope\":\"movie\"") && strstr(corpoA, "\"at\":1757441718"));
  OK(prazoOk && postOk && jsonOk);
  // 401 (c) e 503 (d) ficam no diario para depois; o entregue (a) sai dele
  OK(disco && strstr(disco, "c.test") && strstr(disco, "d.test") && !strstr(disco, "a.test"));
  pthread_mutex_unlock(&tr);

  // segundo evento: a entrega continua; c (401) segue parado, d (backoff) espera
  e = filme(WS_PAUSE, 2000); wsenvio_emitir(&e);
  espera(&nA, 2);
  usleep(600000);
  pthread_mutex_lock(&tr);
  OK(nA == a + 1 && nC == c && nD == d && nB == 0);
  pthread_mutex_unlock(&tr);

  // terceiro evento: o backoff nao pode prender a trava de quem encerra
  e = filme(WS_STOP, 3000);
  t0 = agoraMs(); wsenvio_emitir(&e); dt = agoraMs() - t0;
  OK(dt < 100);
  espera(&nA, 3);
  usleep(600000);
  pthread_mutex_lock(&tr);
  OK(nA == a + 2 && nC == c && nD == d && nB == 0);
  OK(disco && strstr(disco, "|pause|2000|") && strstr(disco, "|stop|3000|"));
  pthread_mutex_unlock(&tr);

  alarm(0);
  if (falhas) { printf("%d FALHAS\n", falhas); return 1; }
  printf("OK watchstate_envio\n");
  return 0;
}
