// #205 race: OK em dois atores seguidos nao pode misturar semente de um com a
// filmografia do outro. rede_baixar falso trava o fio A ate o pedido B chegar.
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define NV_PERFIL_SESSAO_FORTE
const char *desc_chave_tmdb(void) { return "k"; }
const char *desc_tmdb_idioma(void) { return "pt-BR"; }
int ajustes_tmdb_elenco(void) { return 1; }
static pthread_mutex_t portao = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sinal = PTHREAD_COND_INITIALIZER;
static int aberto;
static const char *respPessoa =
  "{\"id\":7,\"name\":\"Ator A\",\"biography\":\"bio A\",\"profile_path\":\"/a.jpg\","
  "\"known_for_department\":\"Acting\",\"combined_credits\":{\"cast\":["
  "{\"id\":1,\"media_type\":\"movie\",\"title\":\"Filme A\",\"popularity\":9}]}}";
char *rede_baixar(const char *u, int t) {
  (void)t;
  if (strstr(u, "/person/7")) {
    pthread_mutex_lock(&portao);
    while (!aberto) pthread_cond_wait(&sinal, &portao);
    pthread_mutex_unlock(&portao);
    return strdup(respPessoa);
  }
  return NULL;  // /find, /credits, /search: nao resolve nada
}
char *rede_baixar_com(const char *u, int t, const char *const *c) {
  (void)c; return rede_baixar(u, t); }
#include "../src/pessoa.c"

int main(void) {
  pessoa_pedir(7, "Ator A", "a.jpg");         // fio A sai, trava no /person/7
  usleep(50 * 1000);
  pthread_mutex_lock(&trava); assert(fioVivo); pthread_mutex_unlock(&trava);
  pessoa_pedir(0, "Ator B", "b.jpg");         // largado: sem clobber
  pthread_mutex_lock(&trava);
  assert(idPedido == 7);                      // nao virou 0
  assert(!strcmp(nomeSemente, "Ator A"));
  assert(!strcmp(nome, "Ator A"));
  pthread_mutex_unlock(&trava);
  pthread_mutex_lock(&portao); aberto = 1; pthread_cond_signal(&sinal);
  pthread_mutex_unlock(&portao);
  usleep(100 * 1000);
  pthread_mutex_lock(&trava);
  assert(pronta && !strcmp(nome, "Ator A") && nCred == 1);
  assert(!strcmp(cred[0].t, "Filme A"));
  pthread_mutex_unlock(&trava);
  // Depois de livre, um pedido sem id pede de novo e a semente troca.
  pessoa_pedir(0, "Ator B", "b.jpg");
  usleep(100 * 1000);
  pthread_mutex_lock(&trava);
  assert(!strcmp(nomeSemente, "Ator B"));     // semente do B registrada
  pthread_mutex_unlock(&trava);
  puts("ok  #205: double-OK nao mistura nome de B com filmografia de A");
  return 0;
}
