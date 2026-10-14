// ITEM 4 DA PESQUISA PLX: recusa no ponto do pedido.
//
// POR QUE ESTE TESTE EXISTE. A pesquisa (docs/pesquisa/plx-native.md, secao 4)
// mediu no aparelho que recusar NO PEDIDO derruba drops de 90/97 para 21/23 e
// sobe a mediana de 55 para 60 fps; "rate-limit depois do decode" e "um em voo"
// ficaram PIORES que nada, porque o pedido e o ultimo ponto onde ainda da para
// recusar de graca. A recusa so pode valer para pedido NOVO de arte FORA da
// janela num quadro em MOVIMENTO: arte conhecida e quadro parado passam (a
// recusa fixa deixava tiles escuros, medido por eles).
//
// Aqui o cache e exercitado sem rede e sem GPU: so o caminho do pedido.
#include "../src/sdlcompat.h"
#ifdef NV_PLX_BEFORE
#include "/Volumes/ExternalSSD/tmp/jobtmp/plx-recusa-before.c"
#else
#include "../src/tex_cache.c"
#endif
#include <assert.h>
#ifdef NV_PLX_BEFORE
// Sem o item 4 na arvore: nada recusa, e o teste tem de FALHAR.
int tex_n_recusa = 0;
void tex_grade_movendo(int m) { (void)m; }
void tex_pedido_fora(int f) { (void)f; }
#endif

static void prepararApi(void) {
  memset(itens, 0, sizeof itens);
  nMax = 8;
  filaIni = filaFim = 0;
  decIni = decFim = 0;
  mtx = SDL_CreateMutex();
  cond = SDL_CreateCond();
  condDec = SDL_CreateCond();
  condLivre = SDL_CreateCond();
  rodando = 1;
  orcamento = 512L * 1024 * 1024;
  assert(mtx && cond && condDec && condLivre);
}

static int ocupados(void) {
  int n = 0;
  for (int i = 0; i < nMax; i++) if (itens[i].estado != VAZIO) n++;
  return n;
}

static int naFila(void) { return filaIni != filaFim; }

int main(void) {
  prepararApi();
  assert(ocupados() == 0 && !naFila());

  // 1) GRADE EM MOVIMENTO + CARTAO FORA DA JANELA: recusado ANTES do fetch.
  //    Nada entra no cache nem na fila de rede.
  tex_grade_movendo(1);
  tex_pedido_fora(1);
  tex_n_recusa = 0;
  assert(tex_obter("https://x/fora.jpg") == 0);
  assert(tex_n_recusa == 1);
  assert(ocupados() == 0 && !naFila());
  tex_pedido_fora(0);

  // 2) GRADE EM MOVIMENTO, CARTAO NA JANELA: admitido normalmente.
  assert(ocupados() == 0);
  tex_n_recusa = 0;
  assert(tex_obter("https://x/na.jpg") == 0);   // ainda sem textura
  assert(tex_n_recusa == 0);
  assert(ocupados() == 1 && naFila());

  // 3) GRADE PARADA: cartao fora da janela NAO e recusado — o lookahead do
  //    item 1 cuida da vizinhanca e o fetch normal cobre o resto.
  tex_grade_movendo(0);
  tex_pedido_fora(1);
  tex_n_recusa = 0;
  assert(tex_obter("https://x/parado.jpg") == 0);
  assert(tex_n_recusa == 0);
  assert(ocupados() == 2);
  tex_pedido_fora(0);

  // 4) ARTE JA CONHECIDA nunca e recusada: e ela que serve a janela. Mesmo em
  //    movimento e fora da janela, o pedido de um item existente passa.
  tex_grade_movendo(1);
  tex_pedido_fora(1);
  tex_n_recusa = 0;
  assert(tex_obter("https://x/na.jpg") == 0);   // ja existe, em voo
  assert(tex_n_recusa == 0);
  assert(ocupados() == 2);
  tex_pedido_fora(0);

  puts("recusa: recusa so no pedido novo, fora da janela, em movimento");
  puts("plx_recusa: PASS");
  return 0;
}
