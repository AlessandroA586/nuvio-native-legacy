#include "p2pmotor.h"
#include "p2p.h"
#include "rede.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char dados[700];

int ajustes_p2p_ligado(void) { return 1; }
const char *ajustes_p2p_url(void) { return ""; }
void debrid_episodio(int *t, int *e) { *t = 0; *e = 0; }
const char *dados_dir(void) { return dados; }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) { (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_postar_st(const char *u, int s, const char *const *c, const char *b, int *st) { (void)u; (void)s; (void)c; (void)b; if (st) *st = 0; return NULL; }
char *rede_baixar_trecho_st(const char *u, int s, long long a, long long b, long *t, int *st, int *e, char *f, unsigned n) {
  (void)u; (void)s; (void)a; (void)b; (void)t; (void)e; (void)f; (void)n; if (st) *st = 0; return NULL; }
int rede_pedir(const RedePedido *p, RedeResposta *r) { (void)p; r->erro = REDE_INDISPONIVEL; return 0; }
void rede_resposta_limpar(RedeResposta *r) { (void)r; }

static void montarUsb(const char *mnt, const char *tmp) {
  char base[700];
  snprintf(base, sizeof base, "%s/nuvio-test", mnt);
  mkdir(mnt, 0755);
  if (symlink(tmp, base) != 0) {
    char cmd[900];
    snprintf(cmd, sizeof cmd, "ln -sfn '%s' '%s'", tmp, base);
    system(cmd);
  }
}

int main(int argc, char **argv) {
  assert(argc > 2);
  snprintf(dados, sizeof dados, "%s/dados", argv[1]);
  mkdir(dados, 0755);
  montarUsb(argv[2], argv[1]);
  p2pmotor_teste_usb_base(argv[2]);

  p2pmotor_definir_usb(1);
  assert(p2pmotor_usb_presente());
  p2pmotor_definir_usb(0);
  puts("ok");
  return 0;
}
