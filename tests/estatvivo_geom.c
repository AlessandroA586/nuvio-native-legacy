// Estatisticas ao vivo: o retangulo do painel de cada tamanho e igual as medidas
// das laterais do player (margem, largura, topo), nao muda com os numeros e o lado
// esquerdo e o espelho do direito. Sem GL. Ver tests/estatvivo_geom.sh.
#include "ajustes.h"
#include "dados.h"
#include "estatvivoui.h"
#include "layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas;
#define CONFERE(c, ...) do { if (!(c)) { printf("FALHA: " __VA_ARGS__); printf("\n"); falhas++; } } while (0)

// As medidas da lateral de legendas/audio: x = 1920 - margem - IL_W (faixas.c),
// o corpo logo abaixo da pilula da hora (plrilha.c: Y_TOPO + PIL_H + vao).
#define LAT_W 720.0f
#define LAT_TOPO (NV_ILHA_Y + 56.0f + 14.0f)

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  int t, i;
  float d[EV_TAM_N][4], e[EV_TAM_N][4], g[4];
  EvEstado est;
  EvTelaTeste tela;
  assert(dir && *dir);
  ajustes_dir(dir);
  dados_iniciar(NULL);

  for (t = 0; t < EV_TAM_N; t++) {
    estatvivo_retangulo(t, 0, d[t]);
    estatvivo_retangulo(t, 1, e[t]);
  }
  CONFERE(fabsf(d[EV_TAM_COMPLETO][2] - LAT_W) < 0.5f, "Completo na largura da lateral: %.1f", d[EV_TAM_COMPLETO][2]);
  CONFERE(fabsf(d[EV_TAM_COMPLETO][0] - (NV_TELA_W - NV_ILHA_MARGEM_D - LAT_W)) < 0.5f, "Completo no x da lateral: %.1f", d[EV_TAM_COMPLETO][0]);
  for (t = 0; t < EV_TAM_N; t++) {
    CONFERE(fabsf(d[t][0] + d[t][2] - (NV_TELA_W - NV_ILHA_MARGEM_D)) < 0.5f, "tam %d: direita na margem", t);
    CONFERE(fabsf(e[t][0] - NV_ILHA_MARGEM_D) < 0.5f, "tam %d: esquerda na margem", t);
    CONFERE(fabsf(d[t][1] - LAT_TOPO) < 0.5f && fabsf(e[t][1] - LAT_TOPO) < 0.5f, "tam %d: abaixo da pilula da hora", t);
    CONFERE(d[t][2] == e[t][2] && d[t][3] == e[t][3], "tam %d: o lado esquerdo e o espelho do direito", t);
    CONFERE(e[t][0] + e[t][2] / 2 + (d[t][0] + d[t][2] / 2) > 0 && fabsf((e[t][0] - NV_ILHA_MARGEM_D) - (NV_TELA_W - NV_ILHA_MARGEM_D - (d[t][0] + d[t][2]))) < 0.5f,
            "tam %d: x espelhado", t);
    CONFERE(d[t][1] + d[t][3] <= NV_TELA_H - 36.0f, "tam %d: cabe na altura da tela (%.0f)", t, d[t][1] + d[t][3]);
  }
  CONFERE(d[EV_TAM_COMPACTO][2] < d[EV_TAM_COMPLETO][2] && d[EV_TAM_COMPACTO][3] < d[EV_TAM_COMPLETO][3], "Compacto menor que o Completo");
  CONFERE(d[EV_TAM_MINIMO][2] < d[EV_TAM_COMPLETO][2] && d[EV_TAM_MINIMO][3] < d[EV_TAM_COMPACTO][3], "Minimo menor que o Compacto");

  // O mesmo retangulo, quadro a quadro, enquanto os numeros mudam (tamanho e lado
  // do painel aberto, nao dos valores).
  for (t = 0; t < EV_TAM_N; t++)
    for (i = 0; i < 40; i++) {
      memset(&est, 0, sizeof est);
      memset(&tela, 0, sizeof tela);
      est.kbps = i * 977 % 90000; est.bufSeg = (double)(i * 13 % 100); est.perdidos = i * 31; est.quadros = 100000 + i;
      snprintf(tela.titulo, sizeof tela.titulo, "%s", i & 1 ? "Titulo curto" : "Um titulo bem mais comprido para forcar a largura do cartao");
      estatvivo_teste(&est, &tela);
      ajustes_definir_estat_lado(i & 1);
      estatvivo_retangulo(t, i & 1, g);
      CONFERE(memcmp(g, (i & 1) ? e[t] : d[t], sizeof g) == 0, "tam %d quadro %d: retangulo muda com os valores", t, i);
    }
  ajustes_definir_estat_lado(0);
  if (falhas) { printf("%d falha(s)\n", falhas); return 1; }
  puts("PASS: estatisticas ao vivo (geometria)");
  return 0;
}
