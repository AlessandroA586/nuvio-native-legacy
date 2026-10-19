// AJ_AMPLIACAO_ALGO: device-local upscaling algorithm selection.
#include "../src/ajustes.c"
#include "nvtmp.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static void write_file(const char *path, const char *content) {
  FILE *f = fopen(path, "w");
  assert(f);
  fputs(content, f);
  fclose(f);
}

int main(void) {
  char dir[NVT_MAX];
  nvt_set(dir, sizeof dir, "/nuvio-aj-algo-XXXXXX");
  assert(mkdtemp(dir));
  char path[700];
  snprintf(path, sizeof path, "%s/ajustes.txt", dir);

  // Test 1: Default value is 0 (Automatico)
  printf("Test 1: Default value is Automatico...");
  ajustes_dir(dir);
  ajustes_iniciar();
  assert(ajustes_ampliacao_algo() == 0);
  printf(" OK\n");

  // Test 2: CHAVE matches enum position
  printf("Test 2: CHAVE matches enum position...");
  assert(strcmp(CHAVE[AJ_AMPLIACAO_ALGO], "ampliarAlgoLocal") == 0);
  printf(" OK\n");

  // Test 3: Setting exists in OPCOES array
  printf("Test 3: Setting in OPCOES array...");
  assert(OPCOES[AJ_AMPLIACAO_ALGO].tipo == OP_ESCOLHA);
  assert(OPCOES[AJ_AMPLIACAO_ALGO].n == 5);  // 5 algorithm options
  printf(" OK\n");

  // Test 4: All enum values fit in expected range
  printf("Test 4: Enum value range...");
  assert(AJ_AMPLIACAO_ALGO >= 0 && AJ_AMPLIACAO_ALGO < AJ_N);
  printf(" OK\n");

  // Cleanup
  snprintf(path, sizeof path, "%s/ajustes.txt", dir);
  unlink(path);
  rmdir(dir);

  printf("\nAll tests passed!\n");
  return 0;
}
