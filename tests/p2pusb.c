// #205: cache do motor P2P em pendrive USB. Monta um "pendrive" de mentira
// (diretorio com um volume gravavel e um somente-leitura), injeta como base
// extra do scan e prova:
//   1. o volume gravavel e escolhido; o somente-leitura nao ganha lixo;
//   2. a raiz escolhida e lembrada e usada pelo resumo (statvfs na mae);
//   3. pendrive removido -> cai para o interno (promessa do ajuste);
//   4. desligar o ajuste apaga a pasta p2p/ deixada no pendrive;
//   5. sem volume nenhum -> usb_presente = 0.
//
//   bash tests/p2pusb.sh
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

// Motor falso minimo: p2pmotor_resumo so precisa de versao().
static const char *vFake(void) { return "teste"; }
static const P2pmOps OPS = { .versao = vFake };

static int existe(const char *p) { struct stat st; return lstat(p, &st) == 0; }

// Espera ate `ms` pelo fio de limpeza apagar `p`. 1 sumiu.
static int someAte(const char *p, unsigned ms) {
  unsigned esperou = 0;
  while (esperou < ms) {
    if (!existe(p)) return 1;
    usleep(20000);
    esperou += 20;
  }
  return 0;
}

int main(int argc, char **argv) {
  char mnt[700], vol[760], ro[760], det[64];
  assert(argc > 2);
  snprintf(dados, sizeof dados, "%s/dados", argv[1]);
  mkdir(dados, 0755);
  snprintf(mnt, sizeof mnt, "%s", argv[2]);
  snprintf(vol, sizeof vol, "%s/vol", mnt);
  snprintf(ro, sizeof ro, "%s/ro", mnt);
  mkdir(vol, 0755);
  mkdir(ro, 0555);
  chmod(mnt, 0555);   // a "raiz do montador" nao serve: o scan desce aos volumes
  p2pmotor_teste_injetar(&OPS, NULL, NULL, NULL, 0);
  p2pmotor_teste_usb_base(mnt);
  p2pmotor_definir_usb(1);

  // 1. volume gravavel escolhido; o somente-leitura nao ganha p2p/ de lixo.
  assert(p2pmotor_usb_presente());
  assert(!existe(strcat(strcpy((char[760]){0}, ro), "/p2p")));

  // 2. a raiz escolhida foi lembrada e o resumo mede statvfs na mae dela.
  assert(p2pmotor_resumo(det, sizeof det) == P2P_OK);

  // 4. desligar apaga a pasta que o scan criou no pendrive.
  {
    char p2pdir[780];
    snprintf(p2pdir, sizeof p2pdir, "%s/p2p", vol);
    assert(existe(p2pdir));   // scan criar=1 (pastas do resumo) a deixou
    p2pmotor_definir_usb(0);
    assert(someAte(p2pdir, 5000));
    p2pmotor_definir_usb(1);
  }

  // 3. pendrive removido no meio: resumo cai para o interno, sem erro.
  chmod(vol, 0555);
  p2pmotor_teste_usb_base(mnt);   // re-scan da mesma base (agora toda 0555)
  assert(p2pmotor_resumo(det, sizeof det) == P2P_OK);
  assert(!p2pmotor_usb_presente());   // vol 0555: nao serve mais

  // 5. base inexistente: nada de pendrive.
  p2pmotor_teste_usb_base("/nao/existe/usb");
  chmod(vol, 0755);
  assert(!p2pmotor_usb_presente());
  p2pmotor_definir_usb(0);
  puts("ok");
  return 0;
}
