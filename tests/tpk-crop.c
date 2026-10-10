// THE SOURCE CROP OF THE .tpk, with no TV.
//
// WHY THIS TEST EXISTS. On the Tizen 5 set the aspect button never moved the
// picture. The log showed why: the old path emulates the crop by drawing the
// whole frame into a LARGER destination shifted off the screen, so a zoom always
// needs a negative origin, and this platform refuses one (`display_win_roi_x out
// of range`; TizenFX #514: "libmm_player don't accept negative position when
// rendering by Overlay"). The host reported success because that is only the
// setter's return code.
//
// The fix is to crop the SOURCE instead: ratios of the frame (never negative)
// and a destination that is the whole screen (never off the screen). This test
// pins the three things that decide whether that works:
//   - the ratios are what the slice is, as a fraction of the frame;
//   - the destination is the whole screen, so nothing leaves it;
//   - when the TV does not have the call, or refuses it, the old path is what
//     runs, and it must not have been silently broken in the meantime.
//
// The host is a fake that records the calls. Build and run:
//   bash tests/tpk-crop.sh
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "video.h"
#include "faixasmkv.h"
#include "mkvass.h"

// --- dependencies video_tpk.c needs, inert --------------------------------
// mkvass_cabecalho and rede_baixar_trecho* come from tests/capmkv_stub.c, which
// is linked in (see tests/tpk-crop.sh): this test must not open a socket.
void mkvass_aceitar_texto(int sim) { (void)sim; }
const char *i18n(const char *s) { return s; }
const char *ling_nome(const char *c) { return c; }
int ling_casa(const char *c, const char *p) { return !strcmp(c, p); }
const char *ling_audio(void) { return ""; }
const char *ling_do_nome(const char *nome) { (void)nome; return NULL; }
int ling_letreiro(const char *nome, int forcado) { (void)nome; (void)forcado; return 0; }
int ling_tipo_legenda(const char *nome) { (void)nome; return 0; }
const char *ling_tipo_legenda_rotulo(int t) { (void)t; return ""; }

// --- the fake host --------------------------------------------------------
void nv_tpk_video_registrar(void (*)(const char *, const char *), void (*)(void), void (*)(int),
                            void (*)(int), void (*)(int), void (*)(int, int, int, int), int (*)(void));
void nv_tpk_video_crop_register(int (*)(double, double, double, double, int, int, int, int));
void nv_tpk_video_crop_available(int tem);
void nv_tpk_video_evento(int tipo, int a, int b);
void nv_tpk_video_faixa(int tipo, int idx, const char *lingua);
void nv_tpk_video_faixas_fim(int selAudio, int selLeg);

static int jx = -1, jy, jw, jh, nj;
static void hAbrir(const char *u, const char *c) { (void)u; (void)c; }
static void hParar(void) {}
static int nPausas, ultimaPausa = -1;
static void hInt(int v) { (void)v; }
static void hPausar(int v) { nPausas++; ultimaPausa = v; }
static void hJanela(int x, int y, int w, int h) { jx = x; jy = y; jw = w; jh = h; nj++; }
static int fakePosMs;
static int  hPos(void) { return fakePosMs; }

// The crop the host was asked for, and whether it was asked at all.
static int nCropos, nZeragens;
static double crx, cry, crw, crh;
static int cdx = -1, cdy, cdw, cdh;
static int zdx = -1, zdy, zdw, zdh;   // the destination the CLEAR was given
static int cropAceita = 1;      // the TV's answer to the crop
static int cropExiste = 1;      // whether this build registers the call at all

static int hRecorte(double rx, double ry, double rw, double rh,
                    int dx, int dy, int dw, int dh) {
  // A crop that covers the whole frame is the "clear it" call, not a crop.
  if (rx == 0.0 && ry == 0.0 && rw == 1.0 && rh == 1.0) {
    nZeragens++; zdx = dx; zdy = dy; zdw = dw; zdh = dh;
    return cropAceita;
  }
  nCropos++;
  crx = rx; cry = ry; crw = rw; crh = rh;
  cdx = dx; cdy = dy; cdw = dw; cdh = dh;
  return cropAceita;
}

static int falhas;
static void checar(int ok, const char *what) {
  printf("%s %s\n", ok ? "ok  " : "FALHA", what);
  if (!ok) falhas++;
}
static void perto(const char *name, double got, double want) {
  int ok = got > want - 0.002 && got < want + 0.002;
  printf("%s %s: %.3f (want %.3f)\n", ok ? "ok  " : "FALHA", name, got, want);
  if (!ok) falhas++;
}
static void limpar(void) {
  nj = 0; jx = -1; nCropos = 0; nZeragens = 0;
  cdx = cdy = cdw = cdh = -1;
  zdx = zdy = zdw = zdh = -1;
}

int main(void) {
  double qw = 1920, qh = 1080;
  nv_tpk_video_registrar(hAbrir, hParar, hPausar, hInt, hInt, hJanela, hPos);
  if (cropExiste) nv_tpk_video_crop_register(hRecorte);
  video_tocar("http://x/filme.mkv");
  nv_tpk_video_evento(6, (int)qw, (int)qh);

  // 1. THE TV HAS THE CROP: the slice goes out as ratios of the frame, and the
  //    destination is the WHOLE SCREEN.
  //
  //    This is the case the report is about: "Zoom leve" on a 16:9 frame asks for
  //    the middle 1670x938 of a 1920x1080 frame with the destination at full
  //    size. Nothing here can be negative, which is the whole point of the fix.
  nv_tpk_video_crop_available(1);
  if (!video_recorte_fonte()) { printf("FALHA: com o recorte a TV deveria oferecer os modos de zoom\n"); falhas++; }
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);
  checar(nCropos == 1, "the source crop was asked for once");
  perto("ratio x",  crx, 124.0 / 1920.0);
  perto("ratio y",  cry,  70.0 / 1080.0);
  perto("ratio w",  crw, 1670.0 / 1920.0);
  perto("ratio h",  crh,  938.0 / 1080.0);
  checar(cdw == 1920 && cdh == 1080, "the crop's destination is the whole screen");
  checar(nj == 0, "with the crop applied the destination is NOT moved (no rectangle off screen)");

  // The failing case: a rectangle with a negative origin must NOT be the answer.
  checar(cdx >= 0 && cdy >= 0, "nothing with a negative origin reaches the plane");

  // 2. A CROP THAT COVERS THE WHOLE FRAME CLEARS IT. Without this the plane stays
  //    zoomed on every later mode that does not crop, which would be a new bug.
  limpar();
  video_janela_fonte(0, 0, (int)qw, (int)qh, 0, 0, 1920, 1080);
  checar(nZeragens == 1, "going back to the whole frame clears the source roi");
  checar(nCropos == 0, "and does not send a whole-frame crop");

  // 3. THE TV REFUSES THE CROP: the old path still has to run, so the picture is
  //    never lost - it just does not zoom. This is the guard that keeps a failed
  //    fix from becoming a black screen.
  limpar();
  cropAceita = 0;
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);
  checar(nCropos == 1, "the refused crop was attempted once");
  checar(nj == 1, "and it fell back to the destination (the picture stays on screen)");

  // 4. THE TV DOES NOT HAVE THE CALL (Tizen 4). Then the player must not offer
  //    the zoom modes at all: on a 16:9 frame five of the eight draw the same
  //    full screen, and the other three would be a button that does nothing.
  cropAceita = 1;
  nv_tpk_video_crop_available(0);
  checar(!video_recorte_fonte(), "with no crop the TV does not offer the zoom modes");

  // 5. CAN IT CHANGE WHILE THE VIDEO IS RUNNING?
  //
  //    The docs say yes on Tizen 5: the ROI may be set in "READY, PLAYING, or
  //    PAUSED"; before 5.0 it had to be Ready or Paused ("no effect if the
  //    player is already in the Playing state"). What this test can pin is the
  //    part that is ours: a crop chosen during playback goes out immediately,
  //    with no pause, no seek and no reload.
  //
  //    That is the whole point of the report - the picture only moved after
  //    leaving and resuming, and resuming is a re-apply with playback already
  //    running.
  nv_tpk_video_crop_available(1);
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);   // mid-playback choice
  checar(nCropos == 1, "a mode chosen mid-playback goes out at once");
  checar(nj == 0, "and without touching the destination (no stop, no reload)");

  // 6. PAUSED FIRST, PLAYING LATER: the crop must still arrive.
  //
  //    Pausing and THEN pressing the aspect key did nothing; playing and then
  //    pausing, the same press worked. The difference is
  //    that the plane only honours a crop once the player has really played -
  //    "asked before playing, the crop was ACCEPTED and IGNORED", the measured note
  //    in src/trailer.c.
  //
  //    The first version of this waited for a FRAME, tested through video_pos(), and
  //    the host's position clock only advances while Playing - so a video left
  //    paused sits at 0.00 s forever, the condition never becomes true, and the crop
  //    is never re-sent. EV_TOCANDO is the sticky fact that covers it.
  video_tocar("http://x/filme.mkv");
  nv_tpk_video_evento(6, (int)qw, (int)qh);
  nv_tpk_video_evento(1, 60000, 0);      // EV_PRONTO: prepared, NEVER played
  fakePosMs = 0;                         // paused at 0.00 s, and it stays there
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);   // the saved mode
  checar(nCropos == 1, "the crop goes out at open");

  limpar();
  video_bombear();
  checar(nCropos == 0, "paused before playing: nothing re-sent yet");

  // THE FILM STARTS with the clock still at 0.00 s - the case that failed: no
  // frame has been decoded yet, but the player has played, and that is enough.
  nv_tpk_video_evento(2, 0, 0);          // EV_TOCANDO
  video_bombear();
  checar(nCropos >= 1, "playing once is enough for the crop to be re-asserted");

  // 6c. "ORIGINAL" KILLS AN OWED ZOOM.
  //
  //     A crop asked for before the film has played is held and re-sent by
  //     video_bombear() (cropSendOwed). Going back to the whole frame does NOT go
  //     through cropSource - it is the video_janela + cropClear path - so it used to
  //     leave that held crop behind, and the next bombear re-sent the STALE ZOOM.
  //     "Original" then looked like it had done nothing when coming back from the
  //     last zoomed mode.
  video_tocar("http://x/terceiro.mkv");   // a new session clears the held state
  nv_tpk_video_evento(6, (int)qw, (int)qh);
  nv_tpk_video_evento(1, 60000, 0);       // EV_PRONTO, never played
  fakePosMs = 0;
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);   // a zoom, now held
  checar(nCropos == 1, "the zoom goes out and is held");

  limpar();
  video_janela_fonte(0, 0, (int)qw, (int)qh, 0, 0, 1920, 1080);   // Original
  checar(nZeragens == 1, "Original clears the crop");

  limpar();
  nv_tpk_video_evento(2, 0, 0);           // the film plays
  video_bombear();
  checar(nCropos == 0, "and the stale zoom is NOT re-sent once there is a picture");

  // 6a-bis. A ZOOM PRESSED WHILE PAUSED IS OWED, AND GOES OUT WHEN THE FILM RUNS.
  //
  //     MEASURED on the TV: a write made while paused is dropped by the plane, and
  //     the picture kept the OLD crop. What heals it is playback starting, so the
  //     retry must wait for the film to be RUNNING. It used to fire on "has it ever
  //     played" (sticky for the session) and was therefore spent - still paused - long
  //     before the person pressed play.
  nv_tpk_video_evento(3, 0, 0);          // EV_PAUSADO: the person pauses
  limpar();
  video_janela_fonte(244, 136, 1432, 806, 0, 0, 1920, 1080);
  checar(nCropos == 1, "paused after playing: the press crops straight away");

  limpar();
  video_bombear();
  checar(nCropos == 0, "while still paused: nothing is re-sent");

  limpar();
  nv_tpk_video_evento(2, 0, 0);          // EV_TOCANDO: playback starts
  video_bombear();
  checar(nCropos == 1, "and starting playback re-sends it (the write lands now)");

  // 6a-ter. THE SAME FOR GOING BACK TO THE WHOLE FRAME.
  //
  //     "going back to Original while paused does nothing, and it stays zoomed even
  //     if I start playback." The clear path was a ONE-SHOT: it had no retry at all,
  //     so a write the plane dropped while paused was lost for good. The zoom path
  //     had a retry for exactly this and the clear path did not.
  nv_tpk_video_evento(3, 0, 0);          // EV_PAUSADO
  limpar();
  video_janela_fonte(0, 0, (int)qw, (int)qh, 0, 0, 1920, 1080);   // Original, paused
  checar(nZeragens == 1, "Original while paused clears the crop straight away");

  limpar();
  nv_tpk_video_evento(2, 0, 0);          // EV_TOCANDO: playback starts
  video_bombear();
  checar(nZeragens == 1, "and starting playback RE-SENDS the whole frame");
  checar(nCropos == 0, "without putting any crop back");

  // 6b. THE CROP HOLDS A PAUSE REQUEST WITH IT. SetVideoRoi resumes the film by
  //     force, so on a paused video the C side asks for the pause again right
  //     after the crop. The host DEFERS that pause while it is using the resume
  //     to get a frame out (Video.cs: Pausar), and pauses itself once the frame
  //     has been presented. The pair going out together is what the host relies
  //     on; if this stops, the repaint either runs the film for its whole budget
  //     or never leaves the pause.
  //
  //     The pause is asked again on the SAME press, so it is one more than the
  //     one EV_PAUSADO above.
  video_pausar(1);                       // the person pauses (what the app does)
  nv_tpk_video_evento(3, 0, 0);          // EV_PAUSADO
  limpar();
  nPausas = 0; ultimaPausa = -1;
  video_janela_fonte(340, 192, 1238, 696, 0, 0, 1920, 1080);
  checar(nCropos == 1, "a paused crop goes out");
  checar(nPausas == 1 && ultimaPausa == 1,
         "and the pause goes out with it (the resume the crop forces is undone)");

  // While PLAYING no pause is invented: the film is already running and the crop
  // is simply the new presentation.
  video_pausar(0);                       // the person pressed play
  nv_tpk_video_evento(2, 0, 0);          // EV_TOCANDO
  limpar();
  nPausas = 0;
  video_janela_fonte(244, 136, 1432, 806, 0, 0, 1920, 1080);
  checar(nCropos == 1, "a playing crop goes out");
  checar(nPausas == 0, "and asks for no pause (the film must keep running)");

  // 7. A NEW VIDEO RE-ARMS IT.
  //
  //    Without this the owed crop belonged to the app's FIRST video, so every later
  //    title carried a stale one.
  video_tocar("http://x/segundo.mkv");   // a new session
  nv_tpk_video_evento(6, (int)qw, (int)qh);
  fakePosMs = 0;
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);
  checar(nCropos == 1, "new video: the crop goes out at open");
  limpar();
  video_bombear();
  nv_tpk_video_evento(2, 0, 0);          // EV_TOCANDO of the new video
  video_bombear();
  checar(nCropos >= 1, "new video: playing once re-asserts it too");

  // 8. A PLAIN WINDOW WRITE KEEPS A ZOOM OFF IT - the PiP corner case.
  //
  //    The corner path is `video_janela` alone (player.c's mini branches): it never
  //    goes through video_janela_fonte, so the crop stayed on the plane and the
  //    corner showed a zoomed slice. Every plain write now clears it - and the
  //    release carries the destination the write was given. A hardcoded full screen
  //    here overwrote the corner (and the credits shrink) the window write before it
  //    had just set.
  limpar();
  video_janela_fonte(124, 70, 1670, 938, 0, 0, 1920, 1080);   // a zoom is in force
  checar(nCropos == 1, "the zoom goes out");
  limpar();
  video_janela(37, 63, 480, 270);                 // the corner (PiP)
  checar(nZeragens == 1, "a plain window write clears the crop off it");
  checar(zdx == 37 && zdy == 63 && zdw == 480 && zdh == 270,
         "and the release carries the destination the write was given");

  // With nothing in force the same write sends nothing: no clear is invented on
  // every reposition. A crop write on a paused video force-resumes it, so silence
  // here is worth keeping.
  limpar();
  video_janela(0, 0, 1920, 1080);
  checar(nZeragens == 0 && nCropos == 0,
         "with no crop in force a plain write sends nothing");

  printf(falhas ? "\ntpk-crop: %d falha(s)\n" : "\ntpk-crop: ok\n", falhas);
  return falhas != 0;
}
