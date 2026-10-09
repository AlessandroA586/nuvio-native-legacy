// #387: um quadro lento entre o KEYDOWN e o KEYUP do OK nao pode virar
// pressao longa. segurar_venceu so dispara depois de um quadro normal.
#include "segurar.h"
#include "layout.h"
#include <stdio.h>

static int falhas;
static void caso(int obtido, int quer, const char *nome) {
  if (obtido != quer) { printf("FAIL %s: %d (quer %d)\n", nome, obtido, quer); falhas++; }
  else printf("ok   %s\n", nome);
}

int main(void) {
  Uint32 t;
  int disparou = 0;
  // Segurando de verdade: quadros de 16 ms ate passar o limiar.
  for (t = 1000; t <= 1000 + NV_HOLD_MS + 32 && !disparou; t += 16)
    disparou = segurar_venceu(t, 1000, "teste");
  caso(disparou, 1, "segurou com quadros normais dispara");
  caso(t - 16 - 1000 >= NV_HOLD_MS && t - 16 - 1000 < NV_HOLD_MS + 32, 1, "dispara logo depois do limiar");
  // Toque curto e um quadro de 900 ms logo depois do KEYDOWN: o KEYUP ja esta
  // na fila e ainda nao foi lido. O quadro que sai da pausa nao pode disparar.
  caso(segurar_venceu(5000, 5000, "teste"), 0, "quadro do KEYDOWN nao dispara");
  caso(segurar_venceu(5900, 5000, "teste"), 0, "quadro depois de pausa de 900 ms nao dispara");
  // Ainda segurando no quadro seguinte (normal): agora dispara.
  caso(segurar_venceu(5916, 5000, "teste"), 1, "quadro normal depois da pausa dispara");
  // Nao armado nunca dispara.
  caso(segurar_venceu(9000, 0, "teste"), 0, "sem armar nao dispara");
  if (falhas) { printf("FAIL #387: %d caso(s)\n", falhas); return 1; }
  puts("PASS #387: pausa longa nao vira pressao longa.");
  return 0;
}
