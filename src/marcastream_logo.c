#include "marcastream.h"
#include "rede.h"
#include "js.h"
#include "descoberta.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_MAX 16
static struct { long rede; atomic_int estado; char url[200]; } slot[MS_MAX];   // 0 livre, 1 buscando, 2 pronto, 3 falhou
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;

static void *fio(void *arg) {
  int k = (int)(long)arg;
  char url[256], *corpo;
  const char *chave = desc_chave_tmdb();
  int ok = 0;
  snprintf(url, sizeof url, "https://api.themoviedb.org/3/network/%ld/images?api_key=%s", slot[k].rede, chave);
  corpo = chave[0] ? rede_baixar(url, 20) : NULL;
  if (corpo) {
    const char *p = js_array(corpo, corpo + strlen(corpo), "logos");
    while (p && !ok) {   // o SVG o SDL_image nao decoda; a lista vem pelos votos
      const char *f = js_fim(p);
      char lp[120] = "";
      if (js_texto(p, f, "file_path", lp, sizeof lp) && lp[0] == '/' && !strstr(lp, ".svg")) {
        snprintf(slot[k].url, sizeof slot[k].url, "https://image.tmdb.org/t/p/w500%s", lp);
        ok = 1;
      }
      p = js_prox(f);
    }
    free(corpo);
  }
  atomic_store(&slot[k].estado, ok ? 2 : 3);
  return NULL;
}

const char *marca_stream_logo(const MarcaStream *m) {
  int k, livre = -1;
  pthread_t t;
  if (!m || m->rede <= 0) return "";
  pthread_mutex_lock(&trava);
  for (k = 0; k < MS_MAX; k++) {
    if (atomic_load(&slot[k].estado) && slot[k].rede == m->rede) break;
    if (livre < 0 && !atomic_load(&slot[k].estado)) livre = k;
  }
  if (k == MS_MAX && livre >= 0) {
    k = livre; slot[k].rede = m->rede;
    atomic_store(&slot[k].estado, 1);
    if (pthread_create(&t, NULL, fio, (void *)(long)k) == 0) pthread_detach(t);
    else atomic_store(&slot[k].estado, 3);
  }
  pthread_mutex_unlock(&trava);
  return k < MS_MAX && atomic_load(&slot[k].estado) == 2 ? slot[k].url : "";
}
