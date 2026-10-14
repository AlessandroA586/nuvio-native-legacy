// O CARTAO DAS ESTATISTICAS AO VIVO no player (mockup stats-velocidade, telas
// 2 e 3, aprovado em 07/10). Abre pelo atalho "Estatisticas" da Central de
// controle; a conta e a origem de cada numero sao de estatvivo.h.
//
// Tres tamanhos: Completo (todas as medidas), Compacto (as principais) e Minimo
// (uma linha fina). O painel usa o vidro e as medidas das laterais do player e
// fica FIXO no tamanho escolhido, no lado da direita ou da esquerda (Ajustes,
// "Lado das estatisticas"). Modal: OK troca o tamanho; Esquerda/Direita trocam o
// lado; Baixo fixa onde esta sem roubar teclas; a Central desafixa. Voltar fecha.
// O tamanho de abertura e o dos Ajustes (ou o ultimo usado). Tudo anda na mola
// (anim.h): abrir/fechar, trocar de tamanho e de lado e os numeros. A amostra corre 1x/s
// enquanto houver video no player, aberto ou nao — e o que da o pico, a media e
// as reconexoes "desde que o filme abriu".
//
// CUSTO NO PLAYER (TCL/Mali sao limitados pela GPU): retangulos solidos, sem
// desfoque nem sombra grande; os textos mudam 1x/s e moram no cache de texto.
#ifndef NV_ESTATVIVOUI_H
#define NV_ESTATVIVOUI_H
#include <SDL2/SDL.h>
#include "estatvivo.h"

enum { EV_TAM_COMPLETO, EV_TAM_COMPACTO, EV_TAM_MINIMO, EV_TAM_N };

void estatvivo_abrir(void);
void estatvivo_fechar(void);
int  estatvivo_aberto(void);       // o cartao grande (modal)
int  estatvivo_fixado(void);       // fixo no tamanho escolhido, sem capturar teclas
int  estatvivo_tamanho(void);      // EV_TAM_*
// Rectangle (x y w h) in the virtual canvas for each size and side: fixed dimensions
// das laterais do player, FIXAS por tamanho (nao dependem de numero nenhum).
void estatvivo_retangulo(int tam, int esquerda, float *g);
void estatvivo_retangulo_atual(float *g);   // do tamanho e do lado de agora
// Teclado do cartao aberto. 1 = consumiu.
int  estatvivo_evento(const SDL_Event *e);
// Uma vez por quadro (barato: so trabalha 1x/s).
void estatvivo_atualizar(Uint32 agora);
void estatvivo_desenhar(Uint32 agora);
// A velocidade mais recente (kbps) para a linha "Tocando agora" da central;
// -1 sem numero. `*estimada` = 1 quando nao e medida.
int  estatvivo_kbps(int *estimada);

#ifdef ESTATVIVO_TESTE
// Capturas: o estado e as linhas do cartao, sem player nem video.
typedef struct {
  char titulo[160], sub[160], addon[64], resolucao[32], codecHdr[64], taxa[24], audio[96], legenda[64];
  int plataforma;   // PtvPlataforma
} EvTelaTeste;
void estatvivo_teste(const EvEstado *e, const EvTelaTeste *t);
#endif

#endif
