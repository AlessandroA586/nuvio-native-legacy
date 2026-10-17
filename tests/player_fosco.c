// FROST OVER LIVE VIDEO (owner, 16/10): with Glass and frost on, the player
// glass over the video hole (control bar, track menu, Statistics panel) must
// read a blur source, not go flat. Prints, per scene, how many glasses had a
// source in its last frame. See tests/player_fosco.sh.
#include "gfx.h"
#include "tex_cache.h"
#include <stdio.h>
static int foscoUlt;
static void fosco_quadro(void) {
  foscoUlt = gfx_vidro_com_fonte;
  gfx_novo_quadro();
}
#define tex_novo_quadro() (fosco_quadro(), tex_novo_quadro())
#define FOSCO_N(id) printf("F %s fonte=%d\n", id, foscoUlt)
#include "player_glass_shot.c"
