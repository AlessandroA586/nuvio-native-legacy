// FROST OVER LIVE VIDEO (owner, 17/10): the player glass over the video hole
// (control bar, track menu, Statistics panel) reads the baked title-art stand-in:
// a blur source, but no frame capture and no blur pass. Prints, per scene, over every frame
// since the scene started: glasses with a source, flat glasses, captures and
// blur passes. See tests/player_fosco.sh.
#include "gfx.h"
#include "tex_cache.h"
#include <stdio.h>
static int foscoFonte, foscoChapado, foscoCap, foscoPas;
static void fosco_quadro(void) {
  foscoFonte += gfx_vidro_com_fonte; foscoChapado += gfx_vidro_chapados;
  foscoCap += gfx_vidro_capturas; foscoPas += gfx_vidro_passadas;
  gfx_novo_quadro();
}
#define tex_novo_quadro() (fosco_quadro(), tex_novo_quadro())
#define FOSCO_ZERA() (foscoFonte = foscoChapado = foscoCap = foscoPas = 0)
#define FOSCO_N(id) printf("F %s fonte=%d chapado=%d cap=%d pas=%d\n", id, foscoFonte, foscoChapado, foscoCap, foscoPas)
#include "player_glass_shot.c"
