// O recuo continuo por quadro deve cancelar Next e proteger o trecho inteiro,
// nao somente os 10 s da primeira tecla. Modulo real, sem janela nem rede.
#include "../src/player.c"
#include <assert.h>

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  Uint32 agora = SDL_GetTicks() + 3000;
  aberto = 1; entrada = 1; comVideo = tocando = 0;
  duracaoSeg = 1500; posSeg = 1450;
  scrubbing = 1; scrubDir = -1; scrubTocava = 0;
  scrubSalto = (SaltoEst){.inicio = agora - 2000, .ultimaTecla = agora, .rep = 1};
  scrubUltimo = agora;
  creditosauto_zerar(&creditosAuto);
  player_atualizar(.05f, agora);
  assert(posSeg < 1450);
  assert(creditosAuto.voltou && creditosAuto.voltouDe == 1450);
  assert(creditosAuto.voltouAte == posSeg);
  IntroTrecho v = {posSeg, 1460, INTRO_CREDITOS}; double fim;
  creditosAuto.buscaPendente = 0;
  assert(!creditosauto_decidir(&creditosAuto, &v, 1, posSeg + 1, 1500, 0, 1, 1, &fim));
  // Continuar avancando nao amplia o intervalo de recuo.
  double ate = creditosAuto.voltouAte;
  scrubDir = 1; scrubSalto.ultimaTecla = agora + 16; scrubUltimo = agora + 16;
  player_atualizar(.05f, agora + 16);
  assert(creditosAuto.voltouAte == ate);
  SDL_Quit();
  puts("player_recuo: recuo continuo protegido; avanco nao amplia a recusa");
  return 0;
}
