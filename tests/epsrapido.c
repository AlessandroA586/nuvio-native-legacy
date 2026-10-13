// Reutiliza os dubles do detalhe; toda a rede e falsa.
#define main detalhe_main
#define rede_baixar detalhe_rede_baixar
#include "detalheanime.c"
#undef rede_baixar
#undef main
#include <time.h>
static double agora(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static char cine[8192], addon[8192];
static int addonMs = 4000, cineMs = 1000;
char *rede_baixar(const char *u, int t) {
  (void)t;
  if (strstr(u, "/meta/series/")) {
    int lento = strstr(u, "addon.test") != NULL;
    usleep((lento ? addonMs : cineMs) * 1000);
    return strdup(lento ? addon : cine);
  }
  return NULL;
}
static void esperar(int i, double inicio) {
  while (!cat_n_episodios(i) && agora() - inicio < 2.0) usleep(10000);
  assert(cat_n_episodios(i) == 20);
  printf("epsrapido: titulo %d publicado em %.3f s\n", i, agora() - inicio);
}
static void fontesDivergentes(void) {
  const char *a = "{\"videos\":[{\"season\":1,\"episode\":1,\"name\":\"A1\"},"
                  "{\"season\":1,\"episode\":2},{\"season\":2,\"episode\":1}]}";
  const char *b = "{\"videos\":[{\"season\":1,\"episode\":1,\"name\":\"B1\"},"
                  "{\"season\":1,\"episode\":2},{\"season\":1,\"episode\":3},"
                  "{\"season\":1,\"episode\":4}]}";
  for (int inversa = 0; inversa < 2; inversa++) {
    catalogoCom("tt0000205", "series", "Divergentes");
    EpPedido p = { .item = 0, .geracao = epGeracao };
    strcpy(p.id, "tt0000205");
    pthread_once(&epPedidoOnce, epPedidoKeyCriar);
    pthread_setspecific(epPedidoKey, &p);
    TempsPub tp = {0};
    publicarEpisodios(inversa ? b : a, 0, "Teste", NULL, 0, &tp);
    CatEp foco = *cat_episodio(0, inversa ? 3 : 2);
    publicarEpisodios(inversa ? a : b, 0, "Teste", NULL, 0, &tp);
    assert(cat_n_episodios(0) == 5);
    const CatEp *e = cat_episodio(0, inversa ? 3 : 2);
    assert(e->temporada == foco.temporada && e->episodio == foco.episodio);
    assert(!strcmp(cat_episodio(0, 0)->nome, inversa ? "A1" : "B1"));
    assert(tp.n == 2 && tp.t[0] == 1 && tp.t[1] == 2);
    pthread_setspecific(epPedidoKey, NULL);
  }
  puts("P1.1: fontes divergentes preservam pares, foco e abas");
}
int main(void) {
  fontesDivergentes();
  CatItem itens[2] = {{0}};
  int nc[] = {20}, na[] = {13};
  corpoSerie(cine, sizeof cine, "tt0000205", nc, 1);
  corpoSerie(addon, sizeof addon, "tt0000205", na, 1);
  // Texto distinguivel para provar a mescla tardia.
  for (char *p = addon; (p = strstr(p, "\"name\":\"E")); p++) p[8] = 'A';
  addonMeta = 1; fakeMetaExterno = 1;
  for (int i = 0; i < 2; i++) {
    snprintf(itens[i].imdb, sizeof itens[i].imdb, "tt000020%d", 5+i);
    strcpy(itens[i].tipo, "series"); strcpy(itens[i].titulo, "Teste");
    origemDaBase(addons_base(0), itens[i].origem, sizeof itens[i].origem);
  }
  cat_definir_tudo(itens, 2, NULL, 0);
  double a = agora(); desc_episodios(0, 1); esperar(0, a);
  usleep(3300000);
  assert(cat_n_episodios(0) == 20);
  assert(!strcmp(cat_episodio(0, 0)->nome, "A1"));
  assert(!strcmp(cat_episodio(0, 19)->nome, "E20"));
  assert(cat_item(0)->nTemporadas == 1);
  for (int i = 0; i < 20; i++) assert(cat_episodio(0, i)->episodio == i + 1);
  limparCacheMeta();
  cat_definir_tudo(itens, 2, NULL, 0);
  a = agora(); desc_episodios(0, 1); usleep(200000);
  double b = agora(); desc_episodios(1, 1); esperar(1, b);
  usleep(3500000);
  assert(cat_n_episodios(0) == 0); // resposta abandonada nao publica
  assert(cat_n_episodios(1) == 20);
  // Ordem inversa: addon primeiro, Cinemeta amplia sem mover os 13 pares.
  limparCacheMeta(); cat_definir_tudo(itens, 2, NULL, 0);
  addonMs = 1000; cineMs = 4000;
  a = agora(); desc_episodios(0, 1);
  while (!cat_n_episodios(0) && agora() - a < 2.0) usleep(10000);
  assert(cat_n_episodios(0) == 13);
  usleep(3300000);
  assert(cat_n_episodios(0) == 20);
  for (int i = 0; i < 20; i++) assert(cat_episodio(0, i)->episodio == i + 1);
  assert(!strcmp(cat_episodio(0, 0)->nome, "A1"));
  puts("epsrapido: mescla, ordem inversa e troca A/B OK");
  return 0;
}
