#include "watchstate_envio.h"
#include "addons.h"
#include "dados.h"
#include "perfis.h"
#include "rede.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARQ_FILA "watchstate_fila.txt"

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  acorda = PTHREAD_COND_INITIALIZER;
static WsFila *fila;
static int     iniciado;

// Com a trava. Diario pequeno (<= WS_FILA_MAX linhas): regravar inteiro basta.
static void gravar(void) {
  char *s = fila->n ? ws_fila_serializar(fila) : NULL;
  if (s) { dados_gravar(ARQ_FILA, s); free(s); }
  else if (!fila->n) dados_apagar(ARQ_FILA);
}

// Chamado por ws_fila_passo COM a trava: solta durante o pedido para o desenho
// poder enfileirar sem esperar os 15 s de um addon fora do ar.
static int enviaReal(const char *url, const char *corpo, int *retryAfterS, void *u) {
  static const char *const cab[] = {"Content-Type: application/json", NULL};
  RedePedido p;
  RedeResposta r;
  int st = 0;
  (void)u;
  memset(&p, 0, sizeof p);
  p.metodo = "POST"; p.url = url; p.cabecalhos = cab;
  p.corpo = corpo; p.n_corpo = strlen(corpo);
  p.prazo_ms = 15000; p.seguir = 0;
  pthread_mutex_unlock(&trava);
  rede_pedir(&p, &r);
  if (r.erro == REDE_OK) st = r.status;   // 4xx/5xx e resposta; 0 = rede ou prazo
  *retryAfterS = r.retry_after_s > 0 ? r.retry_after_s : 0;
  rede_resposta_limpar(&r);
  pthread_mutex_lock(&trava);
  return st;
}

static void *fio(void *arg) {
  (void)arg;
  pthread_mutex_lock(&trava);
  for (;;) {
    long long agora = (long long)time(NULL), prox;
    if (ws_fila_passo(fila, agora, enviaReal, NULL)) gravar();
    prox = ws_fila_proxima(fila);
    if (prox < 0) pthread_cond_wait(&acorda, &trava);
    else {
      struct timespec ate;
      agora = (long long)time(NULL);
      if (prox <= agora) continue;
      ate.tv_sec = (time_t)prox; ate.tv_nsec = 0;
      pthread_cond_timedwait(&acorda, &trava, &ate);
    }
  }
  return NULL;
}

void wsenvio_iniciar(void) {
  pthread_t t;
  char *txt;
  pthread_mutex_lock(&trava);
  if (!iniciado) {
    iniciado = 1;
    fila = calloc(1, sizeof *fila);
    if (fila && (rede_pedido_capacidades() & REDE_CAP_CORPO) &&
        pthread_create(&t, NULL, fio, NULL) == 0) {
      pthread_detach(t);
      if ((txt = dados_ler(ARQ_FILA))) { ws_fila_carregar(fila, txt); free(txt); }
      pthread_cond_signal(&acorda);
    } else { free(fila); fila = NULL; }
  }
  pthread_mutex_unlock(&trava);
}

void wsenvio_emitir(const WsEvento *e) {
  int i, n, novo = 0;
  if (!e || !(rede_pedido_capacidades() & REDE_CAP_CORPO)) return;
  // viewers:false manda o evento para o dono da configuracao do addon. Perfil
  // que usa os addons de outro nao empurra o que ele assiste para esse dono.
  if (perfis_ativo() != perfis_ativo_addons()) return;
  wsenvio_iniciar();
  n = addons_n();
  for (i = 0; i < n; i++) {
    WsDecl d;
    char url[NV_ADDON_PEDIDO_MAX], id[WS_ID_MAX + 64], corpo[1024];
    const char *base = addons_base(i);
    if (!base || !base[0] || !addons_watchstate(i, &d)) continue;
    if (!ws_decl_aceita(&d, e->ev, e->tipo, e->metaId, e->videoId)) continue;
    if (!ws_url(base, e, url, sizeof url) || !ws_id(e, id, sizeof id) ||
        !ws_corpo(e, corpo, sizeof corpo)) continue;
    pthread_mutex_lock(&trava);
    if (fila && ws_fila_empurrar(fila, base, url, id, corpo, (long long)time(NULL))) novo = 1;
    pthread_mutex_unlock(&trava);
  }
  if (novo) {
    pthread_mutex_lock(&trava);
    gravar();
    pthread_cond_signal(&acorda);
    pthread_mutex_unlock(&trava);
  }
}
