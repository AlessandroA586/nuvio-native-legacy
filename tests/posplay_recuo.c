// Reusa o catalogo/roteiro de posplay; exercita o modulo real sem duplicar SDL.
#define main posplay_roteiro
#include "posplay.c"
#undef main

static void abrirContagem(void) {
  posplay_fechar();
  aquecer(3600, 1);
  posplay_atualizar(.016f, 1000, 3597, 3600, 1, 0, 1);
  assert(posplay_visivel());
}
static int pedir(void) {
  int t = 0, e = 0;
  int r = posplay_pediu_episodio(&t, &e);
  if (r) assert(t == 5 && e == 4);
  return r;
}
int main(void) {
  assert(posplay_roteiro() == 0);
  abrirContagem();
  posplay_recuar(); posplay_aguardar_busca(1);
  posplay_atualizar(.016f, 6000, 2000, 3600, 1, 0, 0);
  posplay_atualizar(.016f, 7000, 3600, 3600, 1, 0, 1);
  posplay_aguardar_busca(0);
  posplay_atualizar(.016f, 8000, 3600, 3600, 1, 0, 1);
  assert(posplay_visivel() && !pedir()); // amostra antiga nao rearma Next
  posplay_atualizar(.016f, 9000, 2000, 3600, 1, 0, 0);
  posplay_atualizar(.016f, 10000, 3597, 3600, 1, 0, 1);
  posplay_atualizar(.016f, 13100, 3600, 3600, 1, 0, 1);
  assert(pedir()); // so fora da janela depois do seek permite nova contagem

  abrirContagem();
  posplay_atualizar(.016f, 4100, 3600, 3600, 1, 0, 1);
  assert(!posplay_visivel());
  posplay_recuar(); assert(posplay_visivel() && !pedir());
  assert(tecla(SDLK_RETURN) == 1);
  posplay_recuar(); posplay_aguardar_busca(1);
  assert(pedir() && !pedir()); // OK manual sobrevive ao cancelamento automatico

  abrirContagem();
  posplay_aguardar_busca(1);
  posplay_atualizar(.016f, 6000, 3600, 3600, 1, 0, 1);
  assert(!pedir());
  posplay_aguardar_busca(0);
  posplay_atualizar(.016f, 6100, 3600, 3600, 1, 0, 1);
  assert(pedir()); // fim real libera
  puts("posplay_recuo: contagem, pedido automatico, amostra antiga e OK manual ok");
  return 0;
}
