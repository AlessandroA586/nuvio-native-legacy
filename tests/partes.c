// "Arquivos desta fonte" e o filme em partes (src/partes.h), so a parte pura:
// achar o item da fonte no mylist do TorBox, a ordem, os extras, a parte
// seguinte e a posicao no filme inteiro.
#include "partes.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Rede, disco e TorBox ficam fora deste teste.
int dados_gravar(const char *n, const char *c) { (void)n; (void)c; return 0; }
char *dados_ler(const char *n) { (void)n; return NULL; }
unsigned debrid_tb_geracao(void) { return 1; }
int debrid_tb_disponivel(void) { return 0; }
char *debrid_tb_mylist(int u, int id, int p, int f, int *st) { (void)u; (void)id; (void)p; (void)f; if (st) *st = 0; return NULL; }
int debrid_tb_link(int u, int id, int a, char *url, unsigned n) { (void)u; (void)id; (void)a; (void)url; (void)n; return 0; }
int debrid_tb_adicionar(const char *h) { (void)h; return 0; }
char *rede_baixar_trecho64_final(const char *url, const char *h, int64_t a, int64_t b, long *s,
                                 int64_t *t, int *st, volatile int *c, char *f, int *x) {
  (void)url; (void)h; (void)a; (void)b; (void)s; (void)t; (void)st; (void)c; (void)f; (void)x;
  return NULL;
}

static int falhas;
static void ok(int c, const char *o) { printf("%s %s\n", c ? "ok  " : "FAIL", o); falhas += !c; }

#define H1 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1"
#define H2 "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb2"
static const char *corpo =
  "{\"success\":true,\"data\":["
  "{\"id\":7,\"hash\":\"" H1 "\",\"name\":\"Kaguya-sama First Kiss [Grp]\",\"files\":["
  "{\"id\":10,\"name\":\"K/Kaguya-sama First Kiss - 03.mkv\",\"short_name\":\"Kaguya-sama First Kiss - 03.mkv\",\"size\":300000000},"
  "{\"id\":11,\"name\":\"K/Kaguya-sama First Kiss - 01.mkv\",\"short_name\":\"Kaguya-sama First Kiss - 01.mkv\",\"size\":300000000},"
  "{\"id\":12,\"name\":\"K/Kaguya-sama First Kiss - NCOP.mkv\",\"short_name\":\"Kaguya-sama First Kiss - NCOP.mkv\",\"size\":20000000},"
  "{\"id\":13,\"name\":\"K/Kaguya-sama First Kiss - 02.mkv\",\"short_name\":\"Kaguya-sama First Kiss - 02.mkv\",\"size\":300000000}]},"
  "{\"id\":8,\"hash\":\"" H2 "\",\"name\":\"Other Show [Other]\",\"files\":["
  "{\"id\":20,\"name\":\"O/Other Show - 01.mkv\",\"short_name\":\"Other Show - 01.mkv\",\"size\":500000000}]}"
  "]}";

int main(void) {
  static PartesLista l;
  char hs[2][41];
  int n = partes_hashes_texto("magnet:?xt=urn:btih:" H1 "&dn=x", hs, 2);
  ok(n == 1 && !strcmp(hs[0], H1), "40-hex hash read from the add-on link");

  memset(&l, 0, sizeof l);
  ok(partes_achar(corpo, 0, hs, 1, 1, "", 0, &l) == 1 && l.item == 7, "hash finds the source's item");
  ok(l.n == 4, "all four videos listed");
  ok(l.n == 4 && strstr(l.arq[0].nome, "01") && strstr(l.arq[1].nome, "02") && strstr(l.arq[2].nome, "03"),
     "parts in natural order (01, 02, 03)");
  ok(l.n == 4 && l.arq[3].extra && strstr(l.arq[3].nome, "NCOP"), "NCOP is an extra, after the parts");
  ok(partes_principais(&l) == 3, "three real parts");
  ok(partes_seguinte(&l, 0) == 1 && partes_seguinte(&l, 2) == -1, "next part after 1 is 2; none after 3");
  ok(partes_numero(&l, 1) == 2 && partes_numero(&l, 3) == 0, "part numbers skip the extra");

  { double vp = 0, vd = 0;
    ok(partes_conta_virtual(&l, 1, 600, 1200, 5760, &vp, &vd) && fabs(vd - 5760) < 1 && fabs(vp - 2880) < 1,
       "half of part 2 is the movie's middle");
    ok(!partes_conta_virtual(&l, 3, 10, 60, 5760, &vp, &vd), "an extra has no movie position"); }

  memset(&l, 0, sizeof l);
  ok(partes_achar(corpo, 0, hs, 0, 0, "Kaguya-sama First Kiss - 02.mkv", 300000000, &l) == 1 && l.item == 7 &&
     l.atual >= 0 && strstr(l.arq[l.atual].nome, "02"), "file name finds the item and the playing file");

  { char outro[1][41] = { "cccccccccccccccccccccccccccccccccccccc3" "c" };
    memset(&l, 0, sizeof l);
    ok(partes_achar(corpo, 0, outro, 1, 1, "", 0, &l) != 1, "an unknown hash never shows another release"); }

  printf(falhas ? "partes: %d FAIL\n" : "partes: ok\n", falhas);
  return falhas != 0;
}
