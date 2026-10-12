// Comments episode picker: confirming keeps the focused episode, and the
// first-time hint is only dismissed after the island actually showed it.
#define ilha_mostrou mostrou_fixture
#define extras_n_comentarios com_n_fixture
static int mostrouRet;
static int com_n_fixture(void) { return 2; }
static int mostrou_fixture(const char *chave) { (void)chave; return mostrouRet; }
#include "../src/detail.c"
#undef ilha_mostrou
#undef extras_n_comentarios
#include <assert.h>
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  CatItem c = {0};
  CatEp eps[8] = {{0}};
  int i;
  snprintf(c.imdb, sizeof c.imdb, "tt9999903");
  snprintf(c.tipo, sizeof c.tipo, "series");
  cat_definir_tudo(&c, 1, NULL, 0); idx = 0;
  for (i = 0; i < 8; i++) { eps[i].temporada = 1; eps[i].episodio = i + 1; }
  cat_definir_episodios(0, eps, 8);
  aberto = 1; temporada = 0;
  foco.nColunas[SEC_EPISODIOS] = 8;

  // 1) Right + confirm in the same batch: comments must follow E7, not E6.
  comPicando = 1; foco.fileira = SEC_EPISODIOS; foco.coluna = 5;
  comEpT = 1; comEpE = 6;
  foco.coluna = 6;                       // Right arrived, detail_atualizar not yet run
  comentariosVoltar();
  assert(comEpT == 1 && comEpE == 7);

  // 2) Reopening lands on the confirmed episode, not the old anchor.
  foco.nColunas[SEC_EPISODIOS] = 8;
  comentariosEscolher();
  assert(foco.fileira == SEC_EPISODIOS && foco.coluna == 6);

  // 3) Hint: queued but not shown must not be dismissed; shown must be.
  comPicando = 0; foco.fileira = SEC_COMENTARIOS; foco.coluna = 1; nivel = 1;
  avisodisp_esquecer();
  mostrouRet = 0;
  detail_atualizar(0.016f, SDL_GetTicks());
  assert(!avisodisp_tem(COM_DICA_CHAVE));
  mostrouRet = 1;
  detail_atualizar(0.016f, SDL_GetTicks());
  assert(avisodisp_tem(COM_DICA_CHAVE));
  puts("PASS");
  return 0;
}
