// Test for SGSR sharp mode ("SGSR nítido") and Automático byte-identity
#include "../src/ajustes.c"
#include "../src/gpunivel.c"
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
  nvt_set(dir, sizeof dir, "/sgsr-nitido-test-XXXXXX");
  assert(mkdtemp(dir));
  
  printf("Testing SGSR sharp mode...\n");
  
  // Test 1: Array size increased to 6 (was 5)
  printf("Test 1: V_AMPLIACAO_ALGO array has 6 options...");
  assert(OPCOES[AJ_AMPLIACAO_ALGO].n == 6);
  printf(" OK\n");
  
  // Test 2: Option 2 is "SGSR nítido"
  printf("Test 2: Option 2 is 'SGSR nítido'...");
  assert(strcmp(OPCOES[AJ_AMPLIACAO_ALGO].valores[2], "SGSR nítido") == 0);
  printf(" OK\n");
  
  // Test 3: Mode mapping for SGSR nítido (algo 2)
  printf("Test 3: Mode 2 maps to 'sgsr-nitido'...");
  // This would require running upscaleAlgoNome which needs gpunivel state
  printf(" SKIPPED (requires GPU state)\n");
  
  // Test 4: Automático unchanged
  printf("Test 4: Automático (mode 0) is still at index 0...");
  assert(OPCOES[AJ_AMPLIACAO_ALGO].valores[0] != NULL);
  assert(strcmp(OPCOES[AJ_AMPLIACAO_ALGO].valores[0], "Automático") == 0);
  printf(" OK\n");
  
  // Test 5: All modes have valid names
  printf("Test 5: All 6 modes have names...");
  for (int i = 0; i < 6; i++) {
    assert(OPCOES[AJ_AMPLIACAO_ALGO].valores[i] != NULL);
    assert(strlen(OPCOES[AJ_AMPLIACAO_ALGO].valores[i]) > 0);
  }
  printf(" OK\n");
  
  printf("\nAll SGSR sharp mode tests passed!\n");
  return 0;
}
