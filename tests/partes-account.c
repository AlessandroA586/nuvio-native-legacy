// Exercise real cache/publication paths without network or account secrets.
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../src/partes.c"

static unsigned account = 1;
static int requests, switchDuringRequest;
unsigned debrid_tb_geracao(void) { return account; }
int debrid_tb_disponivel(void) { return 1; }
char *debrid_tb_mylist(int u, int id, int p, int f, int *st) {
  (void)u; (void)id; (void)p; (void)f;
  requests++; *st = 200;
  if (switchDuringRequest) account++;
  return strdup("account files");
}
int debrid_tb_link(int u, int id, int a, char *url, unsigned n) {
  (void)u; (void)id; (void)a; (void)url; (void)n; return 0;
}
int debrid_tb_adicionar(const char *h) { (void)h; return 0; }
int dados_gravar(const char *n, const char *c) { (void)n; (void)c; return 0; }
char *dados_ler(const char *n) { (void)n; return NULL; }
char *rede_baixar_trecho64_final(const char *u, const char *h, int64_t a, int64_t b,
    long *s, int64_t *t, int *st, volatile int *c, char *f, int *x) {
  (void)u; (void)h; (void)a; (void)b; (void)s; (void)t; (void)st; (void)c; (void)f; (void)x;
  return NULL;
}
int main(void) {
  int st = 0;
  char *r = listaConta(0, 0, 0, &st, NULL);
  assert(r && requests == 1); free(r);
  r = listaConta(0, 0, 0, &st, NULL);
  assert(r && requests == 1); free(r);
  account++;
  r = listaConta(0, 0, 0, &st, NULL);
  assert(r && requests == 2); free(r);
  puts("PASS account/key change bypasses account-list cache");

  static Stream source;
  static PartesLista list, out;
  strcpy(source.url, "https://addon.test/same");
  PedidoLista *pd = novoPedido(&source, "title", "source");
  assert(pd);
  pd->geracao = geracaoLista;
  list.estado = PARTES_PRONTO; list.item = 42;
  publicar(pd, &list);
  assert(memoPegarSemTrava("source", &out));
  account++;
  assert(!memoPegarSemTrava("source", &out));
  assert(partes_resultado(&out) == PARTES_VAZIO);
  publicar(pd, &list);
  assert(!memoPegarSemTrava("source", &out));
  assert(partes_resultado(&out) == PARTES_VAZIO);
  free(pd);
  puts("PASS account/key change rejects memo, visible result and stale publication");

  listaEsquecer(); switchDuringRequest = 1;
  r = listaConta(0, 0, 0, &st, NULL);
  assert(!r && st == 0);
  switchDuringRequest = 0;
  int before = requests;
  r = listaConta(0, 0, 0, &st, NULL);
  assert(r && requests == before + 1); free(r);
  listaEsquecer();
  puts("PASS account switch during network rejects response and cache publication");
  return 0;
}
