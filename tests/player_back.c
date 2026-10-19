// Drive the real player handler and app sheet dispatch without a decoder.
#include "video.h"
static unsigned buffering;
static unsigned testBuffering(void) { return buffering; }
#define video_bufferando_ms testBuffering
#include "../src/player.c"
#undef video_bufferando_ms
#include "app.h"
#include <assert.h>

static void reset(void) {
  aberto = visivel = 1; anim = 1.0f;
  saindo = pediuSair = esperandoFonte = erroFonte = comVideo = 0;
  canalSessao = skipFoco = barraFoco = scrubbing = 0;
  buffering = 0;
  pausao_fechar(); posplay_fechar(); reacao_fechar(); intro_desligar();
}

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  assert(ajustes_shot_valor("pauseOverlayEnabled", 0));
  SDL_Keycode keys[] = {SDLK_AC_BACK, SDLK_ESCAPE, SDLK_BACKSPACE, SDLK_DELETE};
  for (unsigned i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
    SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = keys[i];
    reset();
    player_evento(&e);
    assert(!visivel && !saindo && !pediuSair);
    player_evento(&e);
    assert(saindo && pediuSair);

    reset(); visivel = 0; anim = .4f;
    player_evento(&e);
    assert(!visivel && !saindo && !pediuSair);

    reset(); visivel = 0; anim = 0;
    player_evento(&e);
    assert(saindo && pediuSair);

    reset(); esperandoFonte = 1;
    player_evento(&e);
    assert(saindo && pediuSair);

    reset(); buffering = 2000;
    player_evento(&e);
    assert(saindo && pediuSair);

    reset();
    pausao_atualizar(.1f, 1, 1, -1, "", "");
    pausao_atualizar(.1f, 10001, 1, -1, "", "");
    assert(pausao_visivel());
    player_evento(&e);
    assert(!pausao_visivel() && visivel && !saindo && !pediuSair);

    reset(); posplay_shot(-1, 1, 1, 2, 0);
    assert(posplay_visivel());
    if (keys[i] != SDLK_DELETE) {
      player_evento(&e);
      assert(!posplay_visivel() && visivel && !saindo && !pediuSair);
    }

    // App dispatch owns audio, subtitles, speed and sync in the same sheet.
    if (keys[i] != SDLK_DELETE) {
      reset(); faixas_abrir(); assert(faixas_aberta());
      app_evento(&e);
      assert(!faixas_aberta() && visivel && !saindo && !pediuSair);
      reset(); esperandoFonte = 1; faixas_abrir();
      app_evento(&e);
      assert(!faixas_aberta() && !saindo && !pediuSair);
    }
  }
  reset(); skipFoco = 1;
  SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_ESCAPE;
  player_evento(&e);
  assert(!skipFoco && visivel && !saindo);
  reset(); visivel = 0; anim = 0;
  IntroTrecho intro = {10, 100, INTRO_ABERTURA};
  intro_shot_definir(&intro, 1); intro_definir_duracao(1800, 0);
  assert(intro_botao(20, monoSeg(), 0, 0, NULL, NULL));
  player_evento(&e);
  assert(!trechoPulavel(NULL) && !saindo);
  assert(!intro_botao(20, monoSeg(), 0, 0, NULL, NULL));
  player_evento(&e);
  assert(saindo && pediuSair);
  SDL_Quit();
  puts("PASS: Back dismisses one layer; fade consumes Back; pure video/loading exit");
  return 0;
}
