// 2.0.5: a build de cache grande (NV_TEX_MB_FIXO=300) leva quem nunca escolheu
// "Automatico" em Memoria para imagens para 300 MB, UMA vez. Escolha explicita
// fica; uma troca depois da marca fica; a build normal nao muda nada.
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>

static void escreve(const char *dir, const char *linha) {
  char p[1200]; FILE *f;
  snprintf(p, sizeof p, "%s/ajustes.txt", dir);
  f = fopen(p, "w"); assert(f); fputs(linha, f); fclose(f);
}
static void apaga_marca(const char *dir) {
  char p[1200]; snprintf(p, sizeof p, "%s/memoriaimagens-205.txt", dir); unlink(p);
}
int main(void) {
  char dir[1024];
  snprintf(dir, sizeof dir, "%s/nuvio-memoria-XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1); dados_iniciar(dir);
  assert(valorPadrao[AJ_TEX_MB] == 0);                          // fabrica: Automatico
  // Nunca escolheu (0 no arquivo): highcache leva a 300 MB (indice 4), normal fica.
  escreve(dir, "texturasMB 0\n"); ajustes_dir(dir);
#ifdef NV_TEX_MB_FIXO
  assert(valor[AJ_TEX_MB] == 4);
  // Escolheu Automatico depois da migracao: a marca segura, nao volta a 300.
  valor[AJ_TEX_MB] = 0; assert(gravar());
  ajustes_dir(dir); assert(valor[AJ_TEX_MB] == 0);
  // Escolha explicita (96 MB) sem marca nenhuma: nao e sobrescrita.
  apaga_marca(dir); escreve(dir, "texturasMB 1\n"); ajustes_dir(dir);
  assert(valor[AJ_TEX_MB] == 1);
#else
  assert(valor[AJ_TEX_MB] == 0);
  escreve(dir, "texturasMB 1\n"); ajustes_dir(dir);
  assert(valor[AJ_TEX_MB] == 1);
#endif
  printf("ajustes_memoria_migra: ok\n");
  return 0;
}
