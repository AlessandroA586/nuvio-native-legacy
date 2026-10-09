#ifndef NV_SEGURAR_H
#define NV_SEGURAR_H
#include <SDL2/SDL.h>

// SEGURAR OK: a tela arma com o tempo do KEYDOWN e pergunta, a cada quadro,
// se ja passou NV_HOLD_MS sem KEYUP. Antes cada tela fazia so a conta
// `agora - desde >= NV_HOLD_MS`, e um quadro que demorou (decodificacao no fio
// principal, .NET ocupado entregando a tecla no .tpk) fazia o limiar vencer
// ANTES de o KEYUP que ja estava a caminho ser lido: um toque curto virava
// menu do cartaz (#387). Agora o primeiro quadro depois de uma pausa longa
// nunca dispara; o seguinte le os eventos pendentes antes de decidir.
//
// O intervalo entre quadros e medido UMA vez por quadro (app_atualizar chama
// segurar_quadro antes das telas), para que todas as telas que perguntam no
// mesmo quadro vejam a mesma pausa. Sem segurar_quadro (testes que chamam a
// tela direto) a pausa vale 0 e o limiar dispara como antes.
void segurar_quadro(Uint32 agora);

// `agora` e o SDL_GetTicks() do quadro; `desde` o tempo do KEYDOWN (0 = nao
// armado). `tela` so serve ao log, que sai uma vez por pressao.
int segurar_venceu(Uint32 agora, Uint32 desde, const char *tela);

// Intervalo entre dois quadros acima do qual o segurar espera mais um quadro.
#define NV_SEGURAR_PAUSA_MS 150

#endif
