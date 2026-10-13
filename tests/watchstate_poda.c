// watch_state v2: o diario pendente morre com o addon. Compila o addons.c de
// verdade e o fio de entrega, com rede e disco falsos.
//
//   bash tests/watchstate_poda.sh
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "addons.h"
#include "dados.h"
#include "perfis.h"
#include "rede.h"
#include "watchstate_envio.h"

static int falhas;
#define OK(c) do { if (!(c)) { printf("FALHOU %d\n", __LINE__); falhas++; } } while (0)

static pthread_mutex_t tr = PTHREAD_MUTEX_INITIALIZER;
static int posts, caps = REDE_CAP_CORPO;
static char *disco;

unsigned rede_pedido_capacidades(void) { return (unsigned)caps; }
int rede_pedir(const RedePedido *p, RedeResposta *r) {
  (void)p;
  memset(r, 0, sizeof *r);
  pthread_mutex_lock(&tr); posts++; pthread_mutex_unlock(&tr);
  r->erro = REDE_OK; r->status = 503;   // fica pendente de proposito
  return 0;
}
void rede_resposta_limpar(RedeResposta *r) { (void)r; }

int dados_gravar(const char *n, const char *c) { (void)n; pthread_mutex_lock(&tr); free(disco); disco = strdup(c); pthread_mutex_unlock(&tr); return 1; }
char *dados_ler(const char *n) { char *s; (void)n; pthread_mutex_lock(&tr); s = disco ? strdup(disco) : NULL; pthread_mutex_unlock(&tr); return s; }
int dados_apagar(const char *n) { (void)n; pthread_mutex_lock(&tr); free(disco); disco = NULL; pthread_mutex_unlock(&tr); return 1; }
int perfis_ativo(void) { return 1; }
int perfis_ativo_addons(void) { return 1; }

// addons.c pede estes ao mudar a lista; nenhum muda o que se confere aqui.
void fontecache_vod_limpar(void) {}
const char *rede_ultimo_erro(void) { return ""; }
char *rede_baixar(const char *url, int s) { (void)url; (void)s; return NULL; }
char *rede_baixar_trecho_st(const char *url, int s, long long a, long long b,
                            long *tam, int *st, int *erro, char *fin, unsigned tf) {
  (void)url; (void)s; (void)a; (void)b; (void)tam; (void)st; (void)erro; (void)fin; (void)tf;
  return NULL; }
char *rede_baixar_medido_controle(const char *url, int s, const char *const *cab,
                                  const RedeControle *ctl, RedeMedida *m) {
  (void)url; (void)s; (void)cab; (void)ctl; (void)m; return NULL; }
const char *rede_url_publica(const char *url, char *dst, unsigned tam) {
  const char *h, *q; unsigned n;
  if (!dst || !tam) return "";
  dst[0] = 0;
  if (!url) return dst;
  h = strstr(url, "://"); h = h ? h + 3 : url;
  q = strpbrk(h, "/?");
  n = (unsigned)((q ? q : url + strlen(url)) - url);
  if (n >= tam) n = tam - 1;
  memcpy(dst, url, n); dst[n] = 0;
  return dst; }
const char *sessao_usuario(void) { return ""; }
void marco(const char *s) { (void)s; }
int dados_gravar_leve(const char *n, const char *c) { (void)n; (void)c; return 1; }

static const char *MAN =
  "{\"id\":\"com.fixture\",\"name\":\"Fixture\",\"resources\":[\"stream\","
  "{\"name\":\"watch_state\",\"types\":[\"movie\"]}],"
  "\"watchState\":{\"version\":2,\"push\":{\"events\":[\"start\",\"pause\",\"stop\"]}}}";

static WsEvento filme(void) {
  WsEvento e; memset(&e, 0, sizeof e);
  e.ev = WS_STOP; strcpy(e.tipo, "movie");
  strcpy(e.metaId, "tt0133093"); strcpy(e.videoId, "tt0133093");
  e.atMs = 1757441718000LL; e.iniMs = 1757438900000LL; e.posMs = 3000; e.durMs = 8160000;
  e.season = e.episode = -1;
  return e;
}

int main(int argc, char **argv) {
  WsEvento e = filme();
  int i;
  alarm(8);
  addons_carregar("art-inexistente");
  OK(addons_adicionar("Fixture", "https://addon.test/cfg?apikey=fixture"));
  addons_manifesto_lido(0, MAN);
  OK(addons_watchstate(0, &(WsDecl){0}));

  if (argc > 1 && !strcmp(argv[1], "--antes-do-fio")) {
    WsFila f = {0};
    char destino[20];
    ws_destino(addons_base(0), sessao_usuario(), 1, destino);
    OK(ws_fila_empurrar(&f, destino, "/watch_state/push/movie/tt0133093.json",
                         "id", "{}", (long long)time(NULL)));
    disco = ws_fila_serializar(&f); ws_fila_limpar(&f);
    caps = 0; // apagar nao depende de capacidade de POST
    addons_esquecer();
    OK(disco == NULL && posts == 0);
    return falhas ? 1 : 0;
  }

  wsenvio_emitir(&e);
  for (i = 0; i < 200; i++) {
    int gravou;
    pthread_mutex_lock(&tr); gravou = posts == 1 && disco && strstr(disco, "\t1\t"); pthread_mutex_unlock(&tr);
    if (gravou) break;
    usleep(10000);
  }
  pthread_mutex_lock(&tr);
  OK(posts == 1);
  OK(disco && !strstr(disco, "addon.test") && !strstr(disco, "apikey"));
  pthread_mutex_unlock(&tr);

  // Desligar o addon: o pendente sai do diario e nenhum POST novo acontece.
  if (argc > 1 && !strcmp(argv[1], "--esquecer")) addons_esquecer();
  else if (argc > 1 && !strcmp(argv[1], "--remover")) {
    AddonRemoto outro = {"Outro", "https://outro.test/config", 1};
    addons_definir_lista(&outro, 1);
  } else addons_alternar(0);
  usleep(200000);
  pthread_mutex_lock(&tr);
  OK(posts == 1);
  OK(disco == NULL);
  pthread_mutex_unlock(&tr);

  alarm(0);
  if (falhas) { printf("watchstate_poda: %d falha(s)\n", falhas); return 1; }
  printf("watchstate_poda: ok\n");
  return 0;
}
