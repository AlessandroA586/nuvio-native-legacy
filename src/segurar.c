#include "segurar.h"
#include "layout.h"
#include <stdio.h>

// Ultimo quadro que perguntou. Uma tela por vez segura o OK, entao um relogio
// so basta; um valor velho (tela que parou de perguntar) so pode atrasar o
// disparo em um quadro, nunca adianta-lo.
static Uint32 ultimaPergunta;

int segurar_venceu(Uint32 agora, Uint32 desde, const char *tela) {
  Uint32 intervalo = agora - ultimaPergunta;
  ultimaPergunta = agora;
  if (!desde || agora - desde < NV_HOLD_MS) return 0;
  if (intervalo > NV_SEGURAR_PAUSA_MS) {
    // Os eventos que chegaram durante a pausa (o KEYUP de um toque curto) sao
    // lidos antes do proximo quadro; ele decide.
    printf("[segurar] %s: limiar vencido apos pausa de %u ms; espera um quadro\n",
           tela ? tela : "?", (unsigned)intervalo);
    fflush(stdout);
    return 0;
  }
  printf("[segurar] %s: disparou com %u ms segurando\n", tela ? tela : "?",
         (unsigned)(agora - desde));
  fflush(stdout);
  return 1;
}
