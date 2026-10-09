#include "segurar.h"
#include "layout.h"
#include <stdio.h>

// Inicio do quadro atual e do anterior (segurar_quadro).
static Uint32 quadroAnt, quadroAtual;
// Pressao (pelo `desde`) que ja ganhou a linha de log de disparo / de espera.
static Uint32 logadoDisparo, logadoEspera;

void segurar_quadro(Uint32 agora) {
  quadroAnt = quadroAtual;
  quadroAtual = agora;
}

int segurar_venceu(Uint32 agora, Uint32 desde, const char *tela) {
  Uint32 pausa = quadroAnt ? quadroAtual - quadroAnt : 0;
  if (!desde || agora - desde < NV_HOLD_MS) return 0;
  if (pausa > NV_SEGURAR_PAUSA_MS) {
    // Os eventos que chegaram durante a pausa (o KEYUP de um toque curto) sao
    // lidos no inicio do proximo quadro; ele decide.
    if (logadoEspera != desde) {
      logadoEspera = desde;
      printf("[segurar] %s: limiar vencido apos pausa de %u ms; espera um quadro\n",
             tela ? tela : "?", (unsigned)pausa);
      fflush(stdout);
    }
    return 0;
  }
  if (logadoDisparo != desde) {
    logadoDisparo = desde;
    printf("[segurar] %s: disparou com %u ms segurando\n", tela ? tela : "?",
           (unsigned)(agora - desde));
    fflush(stdout);
  }
  return 1;
}
