// Settings default to 100%; migrate old factory sizes once and preserve choices.
#include "../src/ajustes.c"
#include "nvtmp.h"
#include <assert.h>
#include <unistd.h>

static void escreve(const char *dir, const char *linha) {
  char p[1200]; FILE *f;
  snprintf(p, sizeof p, "%s/ajustes.txt", dir);
  f = fopen(p, "w"); assert(f); fputs(linha, f); fclose(f);
}
static void apaga_marca(const char *dir) {
  char p[1200];
  snprintf(p, sizeof p, "%s/ajustesescala-203.txt", dir); unlink(p);
  snprintf(p, sizeof p, "%s/ajustesescala-205.txt", dir); unlink(p);
}
int main(void) {
  char dir[1024];
  snprintf(dir, sizeof dir, "%s/nuvio-escala-XXXXXX", nvt_dir());
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1); dados_iniciar(dir);
  assert(valorPadrao[AJ_TAMANHO_AJUSTES] == 2);
  ajustes_dir(dir); assert(ajustes_tamanho_ajustes() == 1.0f);   // fresh TV
  for (int v = -1; v <= 3; v++) {
    static const float escalas[] = { 1.0f, 0.8f, 0.9f, 1.0f, 1.0f };
    valor[AJ_TAMANHO_AJUSTES] = v;
    assert(ajustes_tamanho_ajustes() == escalas[v + 1]);
  }
  escreve(dir, "tamanhoAjustesLocal 1\n"); ajustes_dir(dir);
  assert(valor[AJ_TAMANHO_AJUSTES] == 2);                       // pre-2.0.3 default
  for (int v = 0; v < 3; v++) {
    valor[AJ_TAMANHO_AJUSTES] = v; assert(gravar());
    ajustes_dir(dir); assert(valor[AJ_TAMANHO_AJUSTES] == v);   // choices after migration
  }
  apaga_marca(dir);
  assert(dados_gravar("ajustesescala-203.txt", "1\n"));
  escreve(dir, "tamanhoAjustesLocal 0\n"); ajustes_dir(dir);
  assert(valor[AJ_TAMANHO_AJUSTES] == 2);                       // 2.0.3 factory default
  ajustes_dir(dir); assert(valor[AJ_TAMANHO_AJUSTES] == 2);      // persisted
  apaga_marca(dir);
  assert(dados_gravar("ajustesescala-203.txt", "1\n"));
  escreve(dir, "tamanhoAjustesLocal 1\n"); ajustes_dir(dir);
  assert(valor[AJ_TAMANHO_AJUSTES] == 1);                       // explicit 90% after 2.0.3
  apaga_marca(dir); escreve(dir, "tamanhoAjustesLocal 0\n"); ajustes_dir(dir);
  assert(valor[AJ_TAMANHO_AJUSTES] == 2);                       // unmarked 80%
  apaga_marca(dir); escreve(dir, "tamanhoAjustesLocal 2\n"); ajustes_dir(dir);
  assert(valor[AJ_TAMANHO_AJUSTES] == 2);                       // explicit 100%
  printf("ajustes_escala_migra: ok\n");
  return 0;
}
