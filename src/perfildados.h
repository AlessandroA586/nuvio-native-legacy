// Snapshot independente de UI para Perfil e Stats. Produtores (Trakt, Simkl)
// preenchem esta estrutura em workers; perfil.c apenas guarda e desenha.
#ifndef NV_PERFILDADOS_H
#define NV_PERFILDADOS_H

#include <stdint.h>

#define PERFIL_MAX_GENEROS    4
#define PERFIL_MAX_DESTAQUES  4
#define PERFIL_MAX_DIAS       42

typedef struct {
  char nome[40];
  int quantidade;
  uint32_t cor;
} PerfilGenero;

typedef struct {
  char id[64];
  char titulo[128];
  char detalhe[96];
  char poster[768];
  char backdrop[768];
  int plays;
  int minutos;
} PerfilDestaque;

typedef struct PerfilDados {
  char nome[96];
  char usuario[64];
  char avatar[768];
  char periodo[48];
  int minutos;
  int plays;
  int filmes;
  int episodios;
  int streakAtual;
  int diasAtivosMes;
  int diasAtivosAno;
  int primeiroDiaSemana;
  int nDias;
  unsigned short atividade[PERFIL_MAX_DIAS];
  int nGeneros;
  PerfilGenero generos[PERFIL_MAX_GENEROS];
  int nDestaques;
  PerfilDestaque destaques[PERFIL_MAX_DESTAQUES];
  int parcial;
  int anoCompleto;
  int streakCompleto;
  char aviso[160];
} PerfilDados;

#endif
