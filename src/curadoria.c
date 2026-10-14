// A curadoria da fileira Herói — o resto do contrato está em curadoria.h.
#include "curadoria.h"
#include "js.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// --- estado -----------------------------------------------------------------

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static char estadoImdb[64];      // "" sem curadoria
static char estadoTexto[240];
static unsigned revisao;
static char url[300];

#ifndef CURADORIA_TESTE
#include "dados.h"
#include "rede.h"
#include <SDL2/SDL_timer.h>
// O ciclo do canal do dono (avisos.c): ler de vez em quando, nunca no fio
// do desenho, nunca mais que uma leitura por vez.
#define NV_CURADORIA_MIN 30
static pthread_t fio;
static int fioVivo;
static Uint32 lidaEm;   // 0 = nunca
#endif

unsigned curadoria_revisao(void) {
  unsigned v;
  pthread_mutex_lock(&trava);
  v = revisao;
  pthread_mutex_unlock(&trava);
  return v;
}

const char *curadoria_imdb(void) {
  const char *r;
  pthread_mutex_lock(&trava);
  r = estadoImdb;
  pthread_mutex_unlock(&trava);
  return r;   // estático: válido até a próxima chamada
}

const char *curadoria_texto(void) {
  const char *r;
  pthread_mutex_lock(&trava);
  r = estadoTexto;
  pthread_mutex_unlock(&trava);
  return r;
}

int curadoria_ativa(void) { return curadoria_imdb()[0] != 0; }

// AS CHAVES QUE O SERVIDOR JA MANDA (rotaReceber, servidor/recomendacoes):
// imdb, tipo, titulo, poster, ano, modelo, texto. Aqui só interessam imdb e
// texto — o título, a arte e o resto vêm do catálogo, que é quem os tem.
static int receberObjeto(const char *ini, const char *fim) {
  char imdb[64] = "", texto[240] = "";
  js_texto(ini, fim, "imdb", imdb, sizeof imdb);
  js_texto(ini, fim, "texto", texto, sizeof texto);
  if (!imdb[0]) return 0;
  pthread_mutex_lock(&trava);
  snprintf(estadoImdb, sizeof estadoImdb, "%s", imdb);
  snprintf(estadoTexto, sizeof estadoTexto, "%s", texto);
  revisao++;
  pthread_mutex_unlock(&trava);
  return 1;
}

int curadoria_receber(const char *corpo) {
  const char *p;
  int ok = 0;
  if (!corpo) return 0;
  if ((p = js_raiz_array(corpo))) {           // array: vale a primeira entrada
    ok = receberObjeto(p, js_fim(p));
  } else if ((p = strchr(corpo, '{')))        // objeto único
    ok = receberObjeto(p, corpo + strlen(corpo));
  if (!ok) {
    // SEM CURADORIA É ESTADO, não é erro: o corpo pode ser um 404 em texto,
    // um {"erro":...} ou o JSON sem imdb — em todos eles a fileira cai na
    // fonte própria e o selo não pode dizer "Curadoria".
    pthread_mutex_lock(&trava);
    if (estadoImdb[0] || estadoTexto[0]) revisao++;
    estadoImdb[0] = 0;
    estadoTexto[0] = 0;
    pthread_mutex_unlock(&trava);
  }
  return ok;
}

// --- a escolha do cartão ------------------------------------------------------

int curadoria_escolher(const int *itens, int n, int catN, int curadoria,
                       unsigned *semente) {
  unsigned s;
  if (curadoria >= 0 && curadoria < catN) return curadoria;
  if (itens && n > 0) return itens[0];
  if (catN < 1) return -1;
  if (!semente) return 0;
  if (*semente) return (int)*semente - 1;   // o sorteio da sessão, guardado
  s = (unsigned)time(NULL);
  *semente = (unsigned)(rand_r(&s) % (unsigned)catN) + 1;
  return (int)*semente - 1;
}

// --- o endereço e o fio de leitura --------------------------------------------

#ifdef CURADORIA_TESTE
// O teste alimenta o parser direto (curadoria_receber); endereço em memória.
int curadoria_definir_url(const char *u) {
  snprintf(url, sizeof url, "%s", u && u[0] ? u : "");
  return 1;
}
const char *curadoria_url(void) { return url; }
void curadoria_passo(void) {}

#else
static void *fioFn(void *u) {
  char *corpo;
  (void)u;
  corpo = rede_baixar_com(url, 15, NULL);
  if (corpo) {
    if (!curadoria_receber(corpo))
      printf("[curadoria] corpo sem recomendacao\n");
    free(corpo);
  }
  fflush(stdout);
  lidaEm = SDL_GetTicks();
  fioVivo = 0;
  return NULL;
}

void curadoria_passo(void) {
  Uint32 agora;
  pthread_mutex_lock(&trava);
  if (!url[0]) { pthread_mutex_unlock(&trava); return; }
  pthread_mutex_unlock(&trava);
  agora = SDL_GetTicks();
  if (fioVivo || (lidaEm && agora - lidaEm < (Uint32)NV_CURADORIA_MIN * 60000u))
    return;
  if (pthread_create(&fio, NULL, fioFn, NULL) == 0) fioVivo = 1;
}

// O endereço mora em curadoria.txt na pasta de dados, por aparelho — o mesmo
// lugar do endereço do servidor P2P (p2p.txt): é do dono da TV, não da conta.
static void carregar(void) {
  char *t = dados_ler("curadoria.txt");
  url[0] = 0;
  if (t) {
    size_t i = 0;
    while (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r') i++;
    snprintf(url, sizeof url, "%s", t + i);
    url[strcspn(url, " \t\n\r")] = 0;
    free(t);
  }
}

const char *curadoria_url(void) {
  static int lido = 0;
  if (!lido) { carregar(); lido = 1; }
  return url;
}

int curadoria_definir_url(const char *u) {
  char nova[sizeof url] = "";
  size_t i = 0;
  while (u && (u[i] == ' ' || u[i] == '\t')) i++;
  if (u && u[i] && strncmp(u + i, "http://", 7) && strncmp(u + i, "https://", 8))
    return 0;
  if (u && u[i]) snprintf(nova, sizeof nova, "%s", u + i);
  pthread_mutex_lock(&trava);
  snprintf(url, sizeof url, "%s", nova);
  pthread_mutex_unlock(&trava);
  if (nova[0]) dados_gravar("curadoria.txt", nova);
  else dados_apagar("curadoria.txt");
  return 1;
}
#endif
