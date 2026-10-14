// ISSUE #513: short OK on an episode, with "Escolher a fonte ao reproduzir"
// on, must not start the saved source. The sheet flag is set in detail.c.
// The router still called tocarFonteGuardada() and handed the live
// fontevolta URL to the player; the sheet only drew on top.
//
// This compiles src/app.c and calls that function (app_teste_tocar_guardada
// is tocarFonteGuardada). player_definir_fonte is the only override: the
// assertion is the URL the real function gives the player.
#include "nvtmp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char urlDoPlayer[128];
static int chamadasFonte, playerAbertoFlag;

#define player_definir_fonte teste_definir_fonte
#define player_aberto teste_player_aberto
#define sessao_usuario teste_sessao_usuario
#define perfis_ativo teste_perfis_ativo
#include "../src/app.c"
#undef player_definir_fonte
#undef player_aberto
#undef sessao_usuario
#undef perfis_ativo

void teste_definir_fonte(const char *u) {
  chamadasFonte++;
  snprintf(urlDoPlayer, sizeof urlDoPlayer, "%s", u ? u : "");
}
int teste_player_aberto(void) { return playerAbertoFlag; }
const char *teste_sessao_usuario(void) { return "conta"; }
int teste_perfis_ativo(void) { return 1; }

static char dir[NVT_MAX], caminho[700];
static void gravar(const char *txt) {
  FILE *f = fopen(caminho, "w");
  assert(f);
  fputs(txt, f);
  fclose(f);
  ajustes_dir(dir);
}

int main(void) {
  CatItem c;
  Stream g;

  nvt_set(dir, sizeof dir, "/nuvio-detail-epfonte-XXXXXX");
  assert(mkdtemp(dir));
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  ajustes_iniciar();

  memset(&c, 0, sizeof c);
  snprintf(c.tipo, sizeof c.tipo, "series");
  snprintf(c.imdb, sizeof c.imdb, "tt0000513");
  c.progresso = 40; c.temporada = 1; c.episodio = 3;
  cat_definir(&c, 1);

  memset(&g, 0, sizeof g);
  snprintf(g.url, sizeof g.url, "https://cdn.invalid/ep3");
  // Same key alvoPlayer builds for T1E3 of this title, live (< 3h).
  // Movie key is the imdb. This series has no episode row loaded, so
  // alvoPlayer resolves the same way: the title, which is what Continue
  // Watching stores for the last session.
  fontevolta_guardar("tt0000513", "conta", 1, &g, 0, SDL_GetTicks());

  // Player open: that is the state when the sheet path used to start the URL.
  playerAbertoFlag = 1;

  // V_LIGA 0 = Ligado. The saved URL must not reach the player.
  gravar("escolherFonteManual 0\n");
  assert(ajustes_fonte_manual());
  urlDoPlayer[0] = 0; chamadasFonte = 0;
  assert(!app_teste_tocar_guardada());
  assert(chamadasFonte == 0 && !urlDoPlayer[0]);
  puts("ok manual: saved source does not start");

  // V_LIGA 1 = Desligado. Resume, including Continue Watching, still reuses it.
  gravar("escolherFonteManual 1\n");
  assert(!ajustes_fonte_manual());
  urlDoPlayer[0] = 0; chamadasFonte = 0;
  assert(app_teste_tocar_guardada());
  assert(chamadasFonte == 1 && !strcmp(urlDoPlayer, g.url));
  puts("ok auto: resume still plays the saved source");

  puts("detail_ep_fonte: saved source waits for the sheet when choose-on-play is on");
  return 0;
}
