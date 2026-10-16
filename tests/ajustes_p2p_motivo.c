// #205: motivo especifico por causa em P2P > Cache do P2P e Limite de espaco.
#include "../src/p2pmotor.h"
static int tDisp, tUsb;
static int tst_disp(void) { return tDisp; }
static int tst_usb(void) { return tUsb; }
#define p2pmotor_disponivel tst_disp
#define p2pmotor_usb_presente tst_usb
#include "../src/ajustes.c"
#include <assert.h>

static void ok(int op, const char *pt) {
  assert(inativa(op));
  assert(!strcmp(ajudaOpcaoBase(op), pt));
}

int main(void) {
  memcpy(valor, valorPadrao, sizeof valor);
  const char *motor = "O motor P2P embutido não está disponível nesta TV.";
  const char *ext = "O cache fica no servidor P2P configurado em Endereço do servidor P2P. Escolha Nesta TV para usar o motor desta TV.";
  const char *usb = "Conecte um pendrive USB com espaço livre na TV para guardar o cache nele. Sem pendrive o cache fica no armazenamento interno.";
  const char *lim = "O limite vale só para o motor desta TV. Com um servidor P2P configurado, quem guarda o cache é o servidor.";
  tDisp = 0; tUsb = 1; p2pEndereco[0] = 0;
  ok(AJ_P2P_DESTINO, motor); ok(AJ_P2P_LIMITE, motor);
  tDisp = 1; snprintf(p2pEndereco, sizeof p2pEndereco, "http://x:1");
  ok(AJ_P2P_DESTINO, ext); ok(AJ_P2P_LIMITE, lim);
  p2pEndereco[0] = 0; tUsb = 0;
  ok(AJ_P2P_DESTINO, usb);
  assert(!inativa(AJ_P2P_LIMITE));
  puts("ajustes_p2p_motivo: ok");
  return 0;
}
