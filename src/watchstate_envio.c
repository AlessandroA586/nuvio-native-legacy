#include "watchstate_envio.h"
#include "addonurl.h"
#include "addons.h"
#include "dados.h"
#include "perfis.h"
#include "rede.h"
#include "sessao.h"
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

typedef struct {
  int sondado;
  char base[NV_ADDON_URL_MAX], destino[20];
  WsDecl decl;
} Destino;

// Com a trava da lista, copia tudo: nenhum ponteiro para um slot sobrevive.
static int copiarDestino(int i, Destino *s) {
  if (perfis_ativo() != perfis_ativo_addons() || !addons_ativo(i)) return 0;
  memset(s, 0, sizeof *s);
  snprintf(s->base, sizeof s->base, "%s", addons_base(i));
  s->sondado = addons_sondado(i);
  addons_watchstate(i, &s->decl);
  ws_destino(s->base, sessao_usuario(), perfis_ativo(), s->destino);
  return 1;
}

// O manifesto pode ainda estar por sondar no arranque.
static int destinoAtual(const char *addon, Destino *s) {
  int i, achou = 0;
  addons_lista_travar();
  for (i = 0; i < addons_n(); i++) {
    if (copiarDestino(i, s) && !strcmp(s->destino, addon)) { achou = 1; break; }
  }
  addons_lista_destravar();
  return achou;
}

static int autorizado(const char *addon, void *u) {
  Destino s;
  (void)u;
  return destinoAtual(addon, &s);
}

// Chamado por ws_fila_passo COM a trava: solta durante o pedido para o desenho
// poder enfileirar sem esperar os 15 s de um addon fora do ar.
static int enviaReal(const char *addon, const char *caminho, const char *corpo,
                     int *retryAfterS, void *u) {
  static const char *const cab[] = {"Content-Type: application/json", NULL};
  RedePedido p;
  RedeResposta r;
  char url[NV_ADDON_PEDIDO_MAX];
  Destino s, atual;
  int st = 0;
  (void)u;
  if (!destinoAtual(addon, &s)) return 410;
  if (!s.decl.push) return s.sondado ? 410 : 0;
  if (s.decl.viewers || nv_addon_url(url, sizeof url, s.base, "%s", caminho) <= 0 ||
      !url[0]) return 410;
  memset(&p, 0, sizeof p);
  p.metodo = "POST"; p.url = url; p.cabecalhos = cab;
  p.corpo = corpo; p.n_corpo = strlen(corpo);
  p.prazo_ms = 15000; p.seguir = 0;
  // A UI pode reordenar ou substituir a lista antes da poda. Revalida a
  // identidade em toda a lista; o pedido usa somente a copia de A.
  if (!destinoAtual(s.destino, &atual) || strcmp(atual.base, s.base)) return 410;
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
    // Addon removido, desligado ou de outra conta: o item sai antes do POST.
    if (ws_fila_podar(fila, autorizado, NULL)) gravar();
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
      if ((txt = dados_ler(ARQ_FILA))) {
        ws_fila_carregar(fila, txt); free(txt);
        // O que o diario trazia de addon que a lista atual nao tem mais nao
        // volta a ser pedido (nem fica no disco).
        ws_fila_podar(fila, autorizado, NULL); gravar();
      }
      pthread_cond_signal(&acorda);
    } else { free(fila); fila = NULL; }
  }
  pthread_mutex_unlock(&trava);
}

void wsenvio_podar(void) {
  pthread_mutex_lock(&trava);
  if (fila && ws_fila_podar(fila, autorizado, NULL)) gravar();
  else if (!fila) {
    // O fio ainda nao nasceu: poda o diario no disco antes de qualquer envio.
    char *txt = dados_ler(ARQ_FILA);
    WsFila f = {0};
    if (txt) { ws_fila_carregar(&f, txt); free(txt); }
    ws_fila_podar(&f, autorizado, NULL);
    { char *s = f.n ? ws_fila_serializar(&f) : NULL;
      if (s) { dados_gravar(ARQ_FILA, s); free(s); }
      else dados_apagar(ARQ_FILA); }
    ws_fila_limpar(&f);
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
  addons_lista_travar();
  n = addons_n();
  addons_lista_destravar();
  for (i = 0; i < n; i++) {
    Destino s;
    int tem;
    char url[NV_ADDON_PEDIDO_MAX], id[WS_ID_MAX + 64], corpo[1024];
    const char *base = s.base;
    addons_lista_travar();
    tem = copiarDestino(i, &s);
    addons_lista_destravar();
    if (!tem || !base[0]) continue;
    if (!ws_decl_aceita(&s.decl, e->ev, e->tipo, e->metaId, e->videoId)) continue;
    if (!ws_url(base, e, url, sizeof url) || !ws_id(e, id, sizeof id) ||
        !ws_corpo(e, corpo, sizeof corpo)) continue;
    // Tira a base e a query; nenhuma credencial entra no diario.
    { size_t lb = strcspn(base, "?");
      char *q;
      while (lb && base[lb - 1] == '/') lb--;
      memmove(url, url + lb, strlen(url + lb) + 1);
      if ((q = strchr(url, '?'))) *q = 0;
    }
    pthread_mutex_lock(&trava);
    if (fila && autorizado(s.destino, NULL) &&
        ws_fila_empurrar(fila, s.destino, url, id, corpo, (long long)time(NULL))) novo = 1;
    pthread_mutex_unlock(&trava);
  }
  if (novo) {
    pthread_mutex_lock(&trava);
    gravar();
    pthread_cond_signal(&acorda);
    pthread_mutex_unlock(&trava);
  }
}
