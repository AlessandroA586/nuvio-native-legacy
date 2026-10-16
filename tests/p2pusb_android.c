#include "p2pmotor.h"
#include "p2p.h"
#include "rede.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

static char dados[700];

static int p2pEnabled = 1;
int ajustes_p2p_ligado(void) { return p2pEnabled; }
const char *ajustes_p2p_url(void) { return ""; }
void debrid_episodio(int *t, int *e) { *t = 0; *e = 0; }
const char *dados_dir(void) { return dados; }
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) { (void)u; (void)s; (void)c; if (st) *st = 0; return NULL; }
char *rede_postar_st(const char *u, int s, const char *const *c, const char *b, int *st) { (void)u; (void)s; (void)c; (void)b; if (st) *st = 0; return NULL; }
char *rede_baixar_trecho_st(const char *u, int s, long long a, long long b, long *t, int *st, int *e, char *f, unsigned n) {
  (void)u; (void)s; (void)a; (void)b; (void)t; (void)e; (void)f; (void)n; if (st) *st = 0; return NULL; }
int rede_pedir(const RedePedido *p, RedeResposta *r) { (void)p; r->erro = REDE_INDISPONIVEL; return 0; }
void rede_resposta_limpar(RedeResposta *r) { (void)r; }

// #205 Android: getExternalFilesDirs candidates. Index 0 (primary) is skipped;
// the writable app dir of a removable volume is accepted AS the drive root,
// even though it shares a device with its parent.
//
//   bash tests/p2pusb_android.sh
static const char *vFake(void) { return "teste"; }
static int criar(const P2pmConfig *c, void **m) { (void)c; (void)m; return -1; }
static const P2pmOps OPS = { .versao = vFake, .criar = criar };
static int existe(const char *p) { struct stat st; return lstat(p, &st) == 0; }

int main(int argc, char **argv) {
  char prim[760], usb[760], lista[1600], mounts[760], p[800], det[64];
  FILE *f;
  assert(argc > 1);
  snprintf(dados, sizeof dados, "%s/dados", argv[1]);
  snprintf(prim, sizeof prim, "%s/primary", argv[1]);
  snprintf(usb, sizeof usb, "%s/usbapp", argv[1]);
  snprintf(mounts, sizeof mounts, "%s/mounts", argv[1]);
  mkdir(dados, 0755); mkdir(prim, 0755); mkdir(usb, 0755);
  f = fopen(mounts, "w");
  fprintf(f, "/dev/root / ext4 rw 0 0\n/dev/x %s vfat rw 0 0\n", usb);
  fclose(f);
  snprintf(lista, sizeof lista, "%s\n%s\n", prim, usb);
  p2pmotor_teste_injetar(&OPS, NULL, NULL, NULL, 0);
  p2pmotor_teste_usb_android(lista, mounts);
  p2pmotor_definir_usb(1);
  assert(p2pmotor_usb_presente());
  assert(p2pmotor_resumo(det, sizeof det) == P2P_OK);
  snprintf(p, sizeof p, "%s/p2p", usb);
  assert(existe(p));                       // app dir used as the root
  snprintf(p, sizeof p, "%s/p2p", prim);
  assert(!existe(p));                      // primary never touched
  // Only the primary listed: nothing removable, no USB.
  snprintf(lista, sizeof lista, "%s\n", prim);
  p2pmotor_teste_usb_android(lista, mounts);
  assert(!p2pmotor_usb_presente());
  printf("p2pusb_android ok\n");
  return 0;
}
