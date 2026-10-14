#ifndef NV_DICASCONTROLE_H
#define NV_DICASCONTROLE_H
#include <SDL2/SDL.h>

// Content version: bump the marker only when a new set of tips ships.
#define DICASCONTROLE_ARQUIVO "dicas-controle-205.txt"
enum { DICAS_TV_CH, DICAS_TV_LG, DICAS_TV_WGT, DICAS_TOQUE };
void dicascontrole_depois_novidades(void);
void dicascontrole_abrir(void);
int dicascontrole_aberto(void);
int dicascontrole_visivel(void);
int dicascontrole_ocupado(void);
int dicascontrole_pagina(void);
int dicascontrole_modo(void);
const char *dicascontrole_tecla(int pagina);
void dicascontrole_evento(const SDL_Event *e);
void dicascontrole_atualizar(float dt, Uint32 agora);
// Draw the underlying screen once at 96px wide, then reuse its blurred copy.
void dicascontrole_fundo(void (*desenhar)(Uint32), Uint32 agora);
void dicascontrole_desenhar(Uint32 agora);
void dicascontrole_encerrar(void);
// Screenshot harness: -1 restores the actual platform/device mode.
void dicascontrole_teste_modo(int modo);
#endif
