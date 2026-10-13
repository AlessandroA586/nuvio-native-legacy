// Scrobble do Simkl a partir do player (issue #459): start / pause / stop com
// porcentagem, contra uma rede FALSA que responde no formato da documentacao
// (api.simkl.org/api-reference/simkl/scrobble-*). Sem SDL, sem rede.
//
// O que a prova cobre: rota e corpo de cada chamada, conta de porcentagem,
// nenhuma chamada sem vinculo ou com guarda recusada, 401 desfaz o vinculo,
// 429/5xx/400-RATE_LIMIT abrem janela de espera (sem tempestade de retry), e
// o stop que falha cai no historico em vez de perder o "visto".
//
// Compilado com -DSMK_SCR_ESPACO_MS=0 -DSMK_SCR_ESPERA_MS=150 para a janela de
// espera caber em milissegundos. STDOUT e capturado e o token nao pode aparecer.
#include "../src/simkl.h"
#include "../src/traktscrobble.h"
#include "../src/ajustes.h"
#include "../src/rede.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TOKEN "TOKEN-SIMKL-SEGREDO-459"
#define OK(s) fprintf(stderr, "ok  %s\n", s)

// ---------------------------------------------------------------- stubs

int ajustes_cw_proximo(void) { return 1; }
int ajustes_cw_minimo(void) { return 1; }
int ajustes_cw_concluido(void) { return 90; }
static const char *tokenAtual = "";
static int nInvalidar;
const char *simklauth_token(void) { return tokenAtual; }
// O real apaga o arquivo do perfil; aqui basta o token sumir.
void simklauth_invalidar(void) { tokenAtual = ""; nInvalidar++; }
const char *nuvem_simkl_cliente(void) { return "cid-teste"; }
const char *nuvem_simkl_app(void) { return "nuvio"; }
void nuvem_url_escapar(const char *v, char *dst, unsigned tam) { snprintf(dst, tam, "%s", v); }
const char *i18n(const char *s) { return s; }
const char *idioma_mes_data(int mes, const char *nomePt) { (void)mes; return nomePt; }
int trakt_enfeitar_lote(CatItem *saida, int n) { (void)saida; return n; }

// ---------------------------------------------------------------- rede falsa

static int posts;                       // POSTs de qualquer rota
static int nScr, nHist;                 // /scrobble/* e /sync/history
static char ultimaRota[64], ultimoCorpo[400], ultimaUrl[700];
static int semBearer;
static int stScr = 200;                 // status devolvido por /scrobble/*
static const char *corpoScr = "{\"id\":1}";

char *rede_postar_st(const char *url, int s, const char *const *cab,
                     const char *corpo, int *st) {
  const char *p = strstr(url, "api.simkl.com");
  size_t n;
  int k, bearer = 0;
  (void)s;
  assert(p);
  p += strlen("api.simkl.com");
  n = strcspn(p, "?");
  assert(n < sizeof ultimaRota);
  memcpy(ultimaRota, p, n); ultimaRota[n] = 0;
  snprintf(ultimaUrl, sizeof ultimaUrl, "%s", url);
  snprintf(ultimoCorpo, sizeof ultimoCorpo, "%s", corpo ? corpo : "");
  assert(strstr(url, "client_id=cid-teste"));
  assert(strstr(url, "app-name=nuvio"));
  assert(strstr(url, "app-version="));
  for (k = 0; cab && cab[k]; k++)
    if (!strcmp(cab[k], "Authorization: Bearer " TOKEN)) bearer = 1;
  if (!bearer) semBearer = 1;
  posts++;
  if (!strncmp(ultimaRota, "/scrobble/", 10)) {
    nScr++;
    *st = stScr;
    return strdup(corpoScr);
  }
  if (!strcmp(ultimaRota, "/sync/history")) nHist++;
  *st = 200;
  return strdup("{\"added\":{},\"not_found\":{\"movies\":[],\"shows\":[]}}");
}

char *rede_baixar_st(const char *url, int s, const char *const *cab, int *st) {
  (void)url; (void)s; (void)cab;
  *st = 200;
  return strdup("{}");
}
char *rede_apagar(const char *url, int s, const char *const *cab, int *st) {
  (void)url; (void)s; (void)cab;
  *st = 204;
  return strdup("");
}

// ---------------------------------------------------------------- ajudantes

static void esperar(void) {
  int i;
  for (i = 0; i < 400 && !simkl_scrobble_ocioso(); i++) usleep(5000);
  assert(simkl_scrobble_ocioso());
}

// Uma chamada e a espera do fio. Devolve a acao decidida; zera os contadores.
static int mandar(int ev, const char *id, double pos, double dur, double ass, int assistiu) {
  int a;
  nScr = nHist = 0;
  a = simkl_scrobble(ev, id, pos, dur, ass, assistiu);
  esperar();
  return a;
}

static void esperarJanela(void) { usleep(700 * 1000); }   // > 2 x SMK_SCR_ESPERA_MS

// ---------------------------------------------------------------- cenarios

static void semVinculo(void) {
  tokenAtual = "";
  posts = 0;
  assert(mandar(SCR_EV_TOCANDO, "tt1375666", 300, 3000, 300, 0) == SCR_NADA);
  assert(mandar(SCR_EV_SAIU, "tt1375666", 3000, 3000, 2700, 1) == SCR_NADA);
  assert(posts == 0);
  OK("sem vinculo nao ha chamada de rede");
  tokenAtual = TOKEN;
}

static void filme(void) {
  assert(mandar(SCR_EV_TOCANDO, "tt1375666", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 1 && !strcmp(ultimaRota, "/scrobble/start"));
  assert(!strcmp(ultimoCorpo, "{\"movie\":{\"ids\":{\"imdb\":\"tt1375666\"}},\"progress\":10.00}"));
  // Mesmo titulo tocando de novo nao repete o start.
  assert(mandar(SCR_EV_TOCANDO, "tt1375666", 330, 3000, 330, 0) == SCR_NADA && nScr == 0);
  assert(mandar(SCR_EV_PAUSOU, "tt1375666", 1000, 3000, 1000, 0) == SCR_PAUSE);
  assert(nScr == 1 && !strcmp(ultimaRota, "/scrobble/pause"));
  // 1000/3000 = 33,33...%: duas casas, o maximo que o Simkl aceita.
  assert(!strcmp(ultimoCorpo, "{\"movie\":{\"ids\":{\"imdb\":\"tt1375666\"}},\"progress\":33.33}"));
  assert(!semBearer);
  OK("filme: start/pause com rota, corpo e porcentagem certos");
}

static void episodio(void) {
  assert(mandar(SCR_EV_TOCANDO, "tt0903747:1:5", 60, 2400, 60, 0) == SCR_START);
  assert(!strcmp(ultimaRota, "/scrobble/start"));
  assert(!strcmp(ultimoCorpo,
    "{\"show\":{\"ids\":{\"imdb\":\"tt0903747\"}},\"episode\":{\"season\":1,\"number\":5},\"progress\":2.50}"));
  assert(mandar(SCR_EV_SAIU, "tt0903747:1:5", 2400, 2400, 2000, 1) == SCR_STOP);
  assert(nScr == 1 && !strcmp(ultimaRota, "/scrobble/stop"));
  assert(!strcmp(ultimoCorpo,
    "{\"show\":{\"ids\":{\"imdb\":\"tt0903747\"}},\"episode\":{\"season\":1,\"number\":5},\"progress\":100.00}"));
  assert(nHist == 0);   // stop aceito: o historico NAO e mandado de novo
  // Saiu antes do fim: pause com o ponto, nunca stop (nao marca visto).
  assert(mandar(SCR_EV_TOCANDO, "tt0903747:1:6", 60, 2400, 60, 0) == SCR_START);
  assert(mandar(SCR_EV_SAIU, "tt0903747:1:6", 1200, 2400, 1200, 0) == SCR_PAUSE);
  assert(!strcmp(ultimaRota, "/scrobble/pause"));
  assert(strstr(ultimoCorpo, "\"progress\":50.00"));
  OK("episodio: show+season/number, stop a 100% e pause ao sair no meio");
}

static void guardas(void) {
  posts = 0;
  assert(mandar(SCR_EV_TOCANDO, "tt2000001", 10, 90, 10, 0) == SCR_NADA);      // < 120 s
  assert(mandar(SCR_EV_TOCANDO, "tt2000002", 10, 3000, 1, 0) == SCR_NADA);     // assistiu < 5 s
  assert(mandar(SCR_EV_TOCANDO, "tmdb:m123", 100, 3000, 100, 0) == SCR_NADA);  // nao e IMDb
  assert(mandar(SCR_EV_TOCANDO, "kitsu:41370:1:2", 100, 3000, 100, 0) == SCR_NADA);
  assert(mandar(SCR_EV_TOCANDO, "", 100, 3000, 100, 0) == SCR_NADA);
  assert(mandar(SCR_EV_SAIU, "tt2000003", 2900, 3000, 200, 0) == SCR_NADA);    // 96% sem ter assistido
  assert(posts == 0);
  OK("guardas do Trakt valem aqui: clipe curto, id sem IMDb, saida >=90% sem assistir");
}

static void indevido(void) {
  // Pausa abaixo de 1% nao e ponto de retomada (mesma regra do Trakt).
  assert(mandar(SCR_EV_TOCANDO, "tt2000004", 10, 3000, 10, 0) == SCR_START);
  assert(mandar(SCR_EV_PAUSOU, "tt2000004", 10, 3000, 10, 0) == SCR_NADA && nScr == 0);
  OK("pause abaixo de 1% nao sai");
}

static void falhaNaoAutorizada(void) {
  int antes = nInvalidar;
  stScr = 401;
  corpoScr = "{\"error\":\"user_token_failed\"}";
  assert(mandar(SCR_EV_TOCANDO, "tt3000001", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 1);
  assert(nInvalidar == antes + 1);
  assert(!simkl_ativo());
  // Desconectado: nada mais sai, e a fila nao fica repetindo.
  posts = 0;
  assert(mandar(SCR_EV_SAIU, "tt3000001", 1500, 3000, 1500, 0) == SCR_NADA);
  assert(mandar(SCR_EV_TOCANDO, "tt3000002", 300, 3000, 300, 0) == SCR_NADA);
  assert(posts == 0);
  OK("401: vinculo desfeito uma vez e nenhuma chamada depois");
  tokenAtual = TOKEN;
  stScr = 200;
  corpoScr = "{\"id\":1}";
}

static void espera(void) {
  stScr = 429;
  assert(mandar(SCR_EV_TOCANDO, "tt4000001", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 1);
  // Dentro da janela: nada sai, nem para outro titulo.
  stScr = 200;
  assert(mandar(SCR_EV_TOCANDO, "tt4000002", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 0);
  assert(simkl_ativo());   // 429 nao desvincula
  esperarJanela();
  assert(mandar(SCR_EV_TOCANDO, "tt4000003", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 1 && !strcmp(ultimaRota, "/scrobble/start"));
  OK("429: janela de espera sem retry, e volta depois dela");

  // 5xx e o 400 RATE_LIMIT do lock de 20 s tambem esperam; 404 nao.
  stScr = 503;
  assert(mandar(SCR_EV_TOCANDO, "tt4000004", 300, 3000, 300, 0) == SCR_START);
  assert(nScr == 1);
  stScr = 200;
  assert(mandar(SCR_EV_TOCANDO, "tt4000005", 300, 3000, 300, 0) == SCR_START && nScr == 0);
  esperarJanela();
  stScr = 400;
  corpoScr = "{\"error\":\"RATE_LIMIT\"}";
  assert(mandar(SCR_EV_TOCANDO, "tt4000006", 300, 3000, 300, 0) == SCR_START && nScr == 1);
  stScr = 200;
  corpoScr = "{\"id\":1}";
  assert(mandar(SCR_EV_TOCANDO, "tt4000007", 300, 3000, 300, 0) == SCR_START && nScr == 0);
  esperarJanela();
  stScr = 404;
  corpoScr = "{\"error\":\"id_err\"}";
  assert(mandar(SCR_EV_TOCANDO, "tt4000008", 300, 3000, 300, 0) == SCR_START && nScr == 1);
  stScr = 200;
  corpoScr = "{\"id\":1}";
  assert(mandar(SCR_EV_TOCANDO, "tt4000009", 300, 3000, 300, 0) == SCR_START && nScr == 1);
  OK("5xx e 400 RATE_LIMIT esperam; 404 nao abre janela");
}

static void stopQueFalha(void) {
  esperarJanela();
  stScr = 500;
  assert(mandar(SCR_EV_TOCANDO, "tt5000001:2:3", 60, 2400, 60, 0) == SCR_START);
  esperarJanela();
  stScr = 500;
  assert(mandar(SCR_EV_SAIU, "tt5000001:2:3", 2400, 2400, 2000, 1) == SCR_STOP);
  assert(nScr == 1 && nHist == 1);
  assert(!strcmp(ultimaRota, "/sync/history"));
  assert(strstr(ultimoCorpo, "\"number\":2") && strstr(ultimoCorpo, "\"number\":3"));
  OK("stop que falha cai no historico (o visto nao se perde)");
  // 409: ja registrado, e sucesso. Sem fallback.
  esperarJanela();
  stScr = 409;
  assert(mandar(SCR_EV_SAIU, "tt5000002", 3000, 3000, 2800, 1) == SCR_STOP);
  assert(nScr == 1 && nHist == 0);
  stScr = 200;
  OK("stop 409 conta como registrado");
}

int main(void) {
  char caminho[] = "/tmp/nuvio-simkl-scr-XXXXXX";
  int fd = mkstemp(caminho), salvo = dup(1);
  FILE *f;
  char linha[512];
  int vazou = 0;
  assert(fd >= 0 && salvo >= 0);
  fflush(stdout);
  dup2(fd, 1);

  semVinculo();
  filme();
  episodio();
  guardas();
  indevido();
  falhaNaoAutorizada();
  espera();
  stopQueFalha();

  fflush(stdout);
  dup2(salvo, 1);
  f = fopen(caminho, "r");
  assert(f);
  while (fgets(linha, sizeof linha, f)) if (strstr(linha, TOKEN)) vazou = 1;
  fclose(f);
  unlink(caminho);
  assert(!vazou);
  OK("o token nunca aparece no stdout do modulo");
  printf("simkl_scrobble: tudo ok\n");
  return 0;
}
