// 2.0.5: highcache migrates an absent memory choice once per device.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

static int falhas;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); falhas++; } } while (0)

static void escreve(const char *dir, const char *linha) {
  char p[1200]; FILE *f;
  snprintf(p, sizeof p, "%s/ajustes.txt", dir);
  f = fopen(p, "w"); assert(f); fputs(linha, f); assert(!fclose(f));
}
static void dispositivo(char *dir, size_t tamanho) {
  snprintf(dir, tamanho, "%s/nuvio-memoria-XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1); dados_iniciar(dir);
  // Older migrations must not accidentally persist the memory migration.
  assert(dados_gravar(RES_ARQ_MIGRADA, "1\n"));
  assert(dados_gravar("envio-151.txt", "1\n"));
  assert(dados_gravar("ajustesescala-203.txt", "1\n"));
  assert(dados_gravar("perfilfundo-20.txt", "1\n"));
  assert(dados_gravar("aparencia-20.txt", "1\n"));
}
static void recarrega(const char *dir) {
  memcpy(valor, valorPadrao, sizeof valor);
  ajustes_dir(dir);
}
static void marca(int esperada) {
  char *m = dados_ler("memoriaimagens-205.txt");
  CHECK((m != NULL) == esperada);
  free(m);
}
int main(void) {
  char dir[1024], outro[1024];
#ifdef NV_TEX_MB_FIXO
  const int migrado = 4, temMarca = 1;
#else
  const int migrado = 0, temMarca = 0;
#endif
  assert(valorPadrao[AJ_TEX_MB] == 0);
  // Explicit Automatic BEFORE migration is a saved choice, just like 96 MB.
  dispositivo(dir, sizeof dir);
  escreve(dir, "texturasMB 0\n"); recarrega(dir);
  CHECK(valor[AJ_TEX_MB] == 0); marca(temMarca);
  dispositivo(dir, sizeof dir);
  escreve(dir, "texturasMB 1\n"); recarrega(dir);
  CHECK(valor[AJ_TEX_MB] == 1); marca(temMarca);
  // An existing file without the key migrates and survives a disk reload.
  dispositivo(dir, sizeof dir);
  escreve(dir, "envioAuto 0\n"); recarrega(dir);
  CHECK(valor[AJ_TEX_MB] == migrado); marca(temMarca);
  recarrega(dir); CHECK(valor[AJ_TEX_MB] == migrado);
  // Changing back to Automatic after migration survives another reload.
  valor[AJ_TEX_MB] = 0; assert(gravar());
  recarrega(dir); CHECK(valor[AJ_TEX_MB] == 0);
  // Even an absent key cannot rerun a completed device migration.
  escreve(dir, "envioAuto 0\n"); recarrega(dir);
  CHECK(valor[AJ_TEX_MB] == 0);
  // A second device has its own marker; no settings file must migrate too.
  dispositivo(outro, sizeof outro);
  marca(0); recarrega(outro);
  CHECK(valor[AJ_TEX_MB] == migrado); marca(temMarca);
  recarrega(outro); CHECK(valor[AJ_TEX_MB] == migrado);
  // Returning to the first device retains its one-time marker.
  setenv("NUVIO_DADOS", dir, 1); dados_iniciar(dir);
  recarrega(dir); CHECK(valor[AJ_TEX_MB] == 0); marca(temMarca);
  printf("ajustes_memoria_migra: %s (%d failures)\n", falhas ? "FAIL" : "ok", falhas);
  return falhas != 0;
}
