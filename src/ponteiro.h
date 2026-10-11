// PONTEIRO DO MAGIC REMOTE (#99) E TOQUE DIRETO (#216).
//
// COMO ELE CHEGA. No webOS o ponteiro e um wl_pointer do compositor, e o SDL
// da LG o entrega como mouse comum: SDL_MOUSEMOTION em coordenadas da janela,
// SDL_MOUSEBUTTONDOWN/UP no clique (o OK do controle com o cursor na tela) e
// SDL_MOUSEWHEEL na rodinha. Alem disso o SDL_webOS.h do SDK define duas
// TECLAS SINTETICAS que nao sao teclas: 484 (SDL_WEBOS_SCANCODE_CURSOR_SHOW) e
// 485 (..._CURSOR_HIDE), mandadas quando o sistema mostra ou esconde o cursor
// (seta apertada, cursor dormiu). E a funcao SDL_webOSCursorVisibility() pede
// ao compositor para mostrar/esconder o cursor dele — o RetroArch chama com 0
// em toda tecla de direcao. Ela e procurada por dlsym: o SDL do aparelho e
// outro binario que o do SDK, e numa firmware sem ela o app nao pode deixar de
// abrir.
//
// A SETA E A DO SISTEMA NO webOS. O SDL da LG so entrega movimento e clique
// com SDL_ShowCursor ligado (MEDIDO na C9, ver main.c), entao la a seta que se
// ve e a do compositor e este modulo nao desenha nenhuma. No Mac o cursor do
// sistema fica desligado e o modulo desenha um circulo proprio.
//
// O QUE ESTE MODULO FAZ. Guarda onde o ponteiro esta (coordenadas LOGICAS,
// 1920x1080, as mesmas do layout) e traduz o ponteiro para o que a interface
// ja entende:
//   - PASSAR POR CIMA foca. Cada tela registra, durante o desenho, os
//     retangulos focaveis (ponteiro_alvo); no movimento seguinte o de cima sob
//     o cursor tem o `focar` chamado — que poe o foco la pela MESMA variavel
//     que as setas mexem.
//   - CLIQUE e OK: foca o alvo e entrega KEYDOWN RETURN no botao descendo e
//     KEYUP no botao subindo. O "segurar" (menu do cartaz) sai sozinho, porque
//     as telas medem a duracao entre os dois. Alvo com `ativar` proprio (a
//     barra de tempo do player, o fundo que fecha uma folha) chama ele em vez
//     do OK.
//   - RODINHA vira seta: cima/baixo, e esquerda/direita na rodinha lateral.
//   - DEDO (#216): tocar = focar + OK; arrastar rola (vira setas, com
//     inercia na soltura); arrastar sobre alvo arrastavel chama o ativar.
//   - SETA DO CONTROLE esconde o cursor; ele volta com um movimento de
//     verdade (janela curta e limiar de distancia: o tremor de quem aperta a
//     seta nao conta). Parado alguns segundos ele some sozinho.
//
// CUSTO ZERO SEM PONTEIRO: com o cursor escondido ponteiro_alvo() retorna na
// primeira linha e nada e desenhado. Quem nunca pega o Magic Remote paga uma
// comparacao por alvo.
//
// CAMADAS. Uma folha ou modal chama ponteiro_camada() antes de desenhar: os
// alvos de tras deixam de valer (so quem tem o teclado pode ter o ponteiro).
// Camada SEM alvo nenhum — uma tela que ainda nao registra nada — recebe o
// clique como OK puro, para o ponteiro nunca deixar a pessoa presa.
#ifndef NV_PONTEIRO_H
#define NV_PONTEIRO_H
#include <SDL2/SDL.h>

typedef void (*PonteiroFn)(int a, int b);

typedef struct {
  float x, y, w, h;
  PonteiroFn focar;    // hover (e antes do OK do clique). Pode ser NULL.
  PonteiroFn ativar;   // clique proprio no lugar do OK. Pode ser NULL.
                       // Os dois NULL = anteparo: absorve o clique, nao faz nada.
  int a, b;
  int arrasta;         // ponteiro_alvo_arrastavel: o dedo arrasta, nao rola
} PonteiroAlvo;

void ponteiro_iniciar(void);
// Diagnostico: loga eventos que nao sao tecla (ver ponteiro.c). Todo evento.
void ponteiro_diag(const SDL_Event *e);

// Filtra um evento do SDL. Devolve 1 quando o evento era do ponteiro e ja foi
// tratado (o chamador nao deve repassa-lo). `entregar` recebe as teclas que o
// ponteiro sintetiza (OK, setas da rodinha).
int  ponteiro_evento(const SDL_Event *e, void (*entregar)(const SDL_Event *));

// Uma vez por quadro, ANTES do desenho: zera a lista que o desenho vai montar
// e faz o cursor dormir por inatividade.
void ponteiro_quadro(Uint32 agora);

// Desenha o cursor, por ultimo no quadro, e FECHA o quadro: a lista montada
// vira a que os eventos do proximo laco consultam. Chamar sempre, mesmo sem
// cursor (sem ele e so a troca de lista).
void ponteiro_desenhar(void);

// Vale a pena registrar alvos? Cursor na tela ou dispositivo de toque. No
// Android os alvos existem antes do primeiro dedo, sem desenhar cursor.
int  ponteiro_ativo(void);

void ponteiro_alvo(float x, float y, float w, float h,
                   PonteiroFn focar, PonteiroFn ativar, int a, int b);
void ponteiro_camada(void);
// O mesmo alvo, recortado a faixa vertical [y0, y1) — a janela de uma grade
// que rola por baixo de um cabecalho fixo. O pedaco do cartao que a rolagem
// escondeu nao recebe o ponteiro (por cima dele mora o cabecalho). E o caminho
// unico das grades: cada tela so diz a sua faixa.
void ponteiro_alvo_faixa(float x, float y, float w, float h, float y0, float y1,
                         PonteiroFn focar, PonteiroFn ativar, int a, int b);
// Marca o ULTIMO alvo registrado como arrastavel por dedo (#216): arrastar
// sobre ele chama o `ativar` a cada movimento, em vez de rolar a tela. E a
// barra de tempo do player.
void ponteiro_alvo_arrastavel(void);
// 1 enquanto focar/ativar estao rodando por causa de um DEDO (e nao do Magic
// Remote). O player usa para tocar = mostrar controles e arrastar = procurar.
int  ponteiro_toque(void);
// Ha tela de toque (Android, ou um dedo ja chegou): alvos pequenos crescem.
int  ponteiro_tem_toque(void);

// When the pointer last moved, by the pointer's own clock (0 = never). A touch
// counts too: a finger resting on the screen is still a hand on the remote.
//
// It exists because motion does NOT reach the player as an event: main.c consumes
// it in the pointer, and inside a single target `mover()` returns early (the hover
// only fires when the target CHANGES), so a hand resting on a button produces
// nothing at all. Whoever needs "is the person still moving the remote?" asks here.
Uint32 ponteiro_last_motion(void);

// Posicao logica atual (para quem ativa por coordenada, como a barra de tempo).
float ponteiro_x(void);
float ponteiro_y(void);

// --- puro, para tests/ponteiro.c ------------------------------------------
// Indice do alvo DE CIMA (o ultimo registrado — ponteiro_camada ja descartou
// os de tras) que contem (x, y); -1 se nenhum.
int  ponteiro_achar(const PonteiroAlvo *v, int n, float x, float y);
// Relogio injetavel: 0 volta ao SDL_GetTicks.
void ponteiro_teste_relogio(Uint32 (*fn)(void));
// Tamanho da janela para a conversao janela -> logico (0 = SDL_GetWindowSize).
void ponteiro_teste_janela(int w, int h);
// Disponibilidade de toque injetavel, sem precisar de hardware no teste.
void ponteiro_teste_toque(int ligado);
// A lista que o hit-test le agora (a do ultimo quadro fechado): os testes de
// tela conferem o que cada tela registrou, sem saber a geometria dela.
int  ponteiro_teste_lista(const PonteiroAlvo **v);
// SDL_webOSCursorVisibility de mentira (so com -DNV_PONT_WEBOS_TESTE).
void ponteiro_teste_cursor_sistema(SDL_bool (*fn)(SDL_bool));

#endif
