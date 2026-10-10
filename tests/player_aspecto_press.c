// THE ASPECT KEY: what the press decided, and what reached the video plane.
//
// WHY THIS TEST EXISTS. Owner report on Tizen 5 (NuvioM .tpk): the aspect
// button does nothing at the moment of the press; the picture only changes
// after leaving playback and resuming. Nothing logged the press, so the pushed
// log could not tell three different things apart - "the key never arrived",
// "the chosen mode draws the rectangle already on screen" and "it went out and
// the plane dropped it" - and each needs a different repair.
//
// This pins what the press now states, with no TV, no GL and no network:
// the entry mode, the chosen mode, both rectangles, and whether the chosen mode
// draws what is already on screen; then the crop that reached the plane, or the
// reason nothing did.
//
// Build and run: bash tests/player_aspecto_press.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "player.h"
#include "catalogo.h"
#include "perfis.h"
#include "ajustes.h"
#include "video.h"
#include <SDL2/SDL.h>

// --- the video plane, as this test sees it ---------------------------------
// The last rectangle handed to the device, split the way the backends split it:
// a plain window, or a crop (source rectangle) plus its destination.
static int janX = -1, janY, janW, janH, nJanelas;
static int fonX = -1, fonY, fonW, fonH, nFontes;
static int dstX = -1, dstY, dstW, dstH;
static int frameW = 1920, frameH = 1080;

void video_janela(int x, int y, int w, int h) {
  janX = x; janY = y; janW = w; janH = h; nJanelas++;
}
void video_janela_fonte(int sx, int sy, int sw, int sh,
                        int dx, int dy, int dw, int dh) {
  fonX = sx; fonY = sy; fonW = sw; fonH = sh;
  dstX = dx; dstY = dy; dstW = dw; dstH = dh; nFontes++;
}
int video_largura(void) { return frameW; }
int video_altura(void)  { return frameH; }

// The source opens. Without this comVideo stays 0 and aplicarAspecto returns
// before it touches the plane - the "[aspect] nothing to apply: no video" line
// the TV log had no way to show. No byte is read: this test touches no network.
int video_tocar(const char *url) { (void)url; return 1; }

// --- the capture -----------------------------------------------------------
static char saida[8192];

// Runs `corpo` with stdout diverted and keeps what it printed.
static void capturar(void (*corpo)(void)) {
  int fd = dup(1);
  FILE *f = tmpfile();
  saida[0] = 0;
  if (fd < 0 || !f) { corpo(); return; }
  fflush(stdout);
  dup2(fileno(f), 1);
  corpo();
  fflush(stdout);
  dup2(fd, 1);
  close(fd);
  rewind(f);
  { size_t n = fread(saida, 1, sizeof saida - 1, f); saida[n] = 0; }
  fclose(f);
}
static int contem(const char *agulha) { return strstr(saida, agulha) != NULL; }

static int falhas;
static void checar(int ok, const char *o_que) {
  printf("%s %s\n", ok ? "ok  " : "FALHA", o_que);
  if (!ok) falhas++;
}

static void tecla(void) { player_aspecto_ciclar(); }

// Opens the player the way the home would, so aplicarAspecto has a plane. The
// source does not exist on purpose: nothing here depends on the network, and
// the frame size is answered by this file's own video_largura/video_altura.
static void abrir(void) {
  CatItem c = {0};
  snprintf(c.imdb, sizeof c.imdb, "fixture-aspecto");
  snprintf(c.titulo, sizeof c.titulo, "Titulo ficticio");
  snprintf(c.tipo, sizeof c.tipo, "movie");
  cat_definir(&c, 1);
  player_abrir(0, "https://example.invalid/aspecto.mkv");
  player_atualizar(.016f, SDL_GetTicks());
}

int main(void) {
  char cam[600];
  const char *d = getenv("NUVIO_DADOS");
  printf("player_aspecto_press: inicio\n");
  SDL_Init(SDL_INIT_TIMER);
  perfis_definir_ativo(1);
  // "Last used" as the default mode, so the press starts from the file and the
  // test does not depend on the machine's saved preference.
  if (d && *d) {
    snprintf(cam, sizeof cam, "%s/ajustes.txt", d);
    { FILE *f = fopen(cam, "w"); if (f) { fputs("proporcaoPadrao 1\n", f); fclose(f); } }
    ajustes_dir(d);
    player_dir(d);
  }
  abrir();

  // A 16:9 FRAME: Original, Recortar, Esticar, Fit altura and Fit largura ALL
  // compute 0,0 1920x1080 - fitting, covering and filling are the same drawing
  // when the frame is the size of the screen. Those five are the modes that
  // "do nothing yet are displayed", so the cycle must not stop on them.
  frameW = 1920; frameH = 1080;
  player_aspecto_definir(PLR_ASP_ORIGINAL);
  capturar(tecla);
  fputs(saida, stdout);
  checar(contem("[aspect] key: mode 0 (Original) -> 3 (Zoom leve);"),
         "the cycle skips modes that draw the same rectangle (no wasted press)");
  checar(!contem("-> 1 (Recortar)"),
         "and does not stop on a mode that changes nothing");

  // A real change: "Zoom cinema" crops the source, so the source rectangle that
  // reaches the plane is SMALLER than the frame.
  player_aspecto_definir(PLR_ASP_ZOOM_LEVE);
  nFontes = 0; fonX = -1;
  capturar(tecla);                    // ZOOM_LEVE -> ZOOM_CINEMA
  fputs(saida, stdout);
  checar(contem("[aspect] mode=4 frame 1920x1080: source"),
         "a cropping mode reports the frame, the source and the destination");
  checar(!contem("SAME as before"), "a cropping mode is not announced as unchanged");
  checar(nFontes == 1, "a cropping mode hands the source to the plane");
  checar(fonW > 0 && fonW < frameW && fonH < frameH,
         "the zoom crops the source (a slice smaller than the frame)");
  checar(dstX == 0 && dstY == 0 && dstW == 1920 && dstH == 1080,
         "the destination stays the whole screen (the source is what changed)");

  // STARTING FROM A MODE THAT HAS NO DOT OF ITS OWN. Fit largura and Fit altura
  // draw what Original draws on a 16:9 frame, so neither is its group's
  // representative. The scan used to break on Original, see the same rectangle
  // and refuse the press there - the key was dead for anyone whose saved mode was
  // one of these two. It must walk past the current drawing to the zooms.
  frameW = 1920; frameH = 1080;
  player_aspecto_definir(PLR_ASP_FIT_LARGURA);
  capturar(tecla);
  fputs(saida, stdout);
  checar(contem("-> 3 (Zoom leve)"),
         "from Fit largura the key reaches a mode that changes the picture");

  player_aspecto_definir(PLR_ASP_FIT_ALTURA);
  capturar(tecla);
  fputs(saida, stdout);
  checar(contem("-> 3 (Zoom leve)"),
         "from Fit altura the key reaches a mode that changes the picture");

  // And the key must not turn into a spinner when it comes back around: from the
  // last zoom the lap ends at Original, which draws something else again.
  player_aspecto_definir(PLR_ASP_ZOOM_ULTRA);
  capturar(tecla);
  fputs(saida, stdout);
  checar(contem("-> 0 (Original)"),
         "the lap from the last zoom lands on Original, not on the same drawing");

  printf("\n%s (%d failure(s))\n", falhas ? "FAILED" : "all ok", falhas);
  return falhas ? 1 : 0;
}
