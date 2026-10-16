// Exercise the actual preference reader, Settings controls and overlay RGB resolver.
#include "../src/ajustes.c"
#include <assert.h>

static char dirPrefs[512];
static int prefsLidas, aspecto;
const char *const VIDEO_LEG_CORES[VIDEO_LEG_NCORES] = {"White", "Yellow", "Green", "Blue", "Red", "Grey"};
const char *const VIDEO_LEG_CORES_PT[VIDEO_LEG_NCORES] = {"White", "Yellow", "Green", "Blue", "Red", "Grey"};
char *SDL_GetBasePath(void) { return NULL; }
void SDL_free(void *p) { free(p); }
void dados_marcar_sujo(int imediato) { (void)imediato; }
void video_legenda_estilo(const VideoLegendaEstilo *e) { (void)e; }
#include "prefs.inc"

int main(int argc, char **argv) {
  assert(argc == 2);
  valor[AJ_LEG_CONTORNO_COR] = 3; // Yellow saved by 2.0.5.
  char path[600];
  snprintf(path, sizeof path, "%s/player.txt", argv[1]);
  FILE *f = fopen(path, "w");
  assert(f);
  fputs("leg_cor_borda 0\nleg_tocado 0\n", f); // Old player saved default Black.
  assert(fclose(f) == 0);
  player_dir(argv[1]);
  player_leg_estilo_carregar();
  assert(ajustes_leg_borda_rgb(player_leg_estilo()->corBorda) == 0xffde30u);
  // Unrelated edits must preserve the legacy colour, including after reload.
  assert(ajLegendaDefinir(AJ_LEG1_TAMANHO, 140));
  player_dir(argv[1]);
  player_leg_estilo_carregar();
  assert(ajustes_leg_borda_rgb(player_leg_estilo()->corBorda) == 0xffde30u);
  VideoLegendaEstilo secondary;
  valor[AJ_LEG2_COR_BORDA] = 1; // Explicit Black while primary remains legacy Yellow.
  ajustes_leg2_estilo(player_leg_estilo(), &secondary);
  assert(ajustes_leg_borda_rgb(secondary.corBorda) == 0);
  assert(ajLegendaDefinir(AJ_LEG1_COR_BORDA, 0));
  assert(ajustes_leg_borda_rgb(player_leg_estilo()->corBorda) == 0);
  player_dir(argv[1]);
  player_leg_estilo_carregar();
  assert(ajustes_leg_borda_rgb(player_leg_estilo()->corBorda) == 0);
  valor[AJ_LEG2_COR_BORDA] = 0; // Follow the explicit primary Black.
  ajustes_leg2_estilo(player_leg_estilo(), &secondary);
  assert(ajustes_leg_borda_rgb(secondary.corBorda) == 0);
  // Existing explicit player colours also survive upgrade.
  f = fopen(path, "w"); assert(f);
  fprintf(f, "leg_cor_borda 0\nleg_tocado %d\n", PLR_LEG_COR_BORDA);
  assert(fclose(f) == 0);
  player_dir(argv[1]); player_leg_estilo_carregar();
  assert(ajustes_leg_borda_rgb(player_leg_estilo()->corBorda) == 0);
  puts("legenda_borda_preta: PASS");
  return 0;
}
