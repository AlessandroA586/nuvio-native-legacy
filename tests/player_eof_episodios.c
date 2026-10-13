// Reuse the host video pipeline and playback fixture; no window or network.
#define main visto_trailer_main
#include "player_visto_trailer.c"
#undef main
#include "posplay.h"

int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0);
  dados_iniciar(""); ling_local_legenda("none");
  ajustes_shot_valor("proximoAutoLocal", 0);
  assert(ajustes_proximo_auto());

  // Empty/loading, empty/unknown, and a partial list still loading.
  for (int caso = 0; caso < 3; caso++) {
    CatItem c = {0}; CatEp eps[2] = {{0}};
    int t = 0, e = 0;
    snprintf(c.imdb, sizeof c.imdb, "tt9000038%d", caso);
    strcpy(c.tipo, "series"); c.temporada = c.episodio = 1;
    cat_definir(&c, 1);
    eps[0].temporada = eps[0].episodio = 1;
    eps[1].temporada = 1; eps[1].episodio = 2;
    if (caso == 2) cat_definir_episodios(0, eps, 1);
    carregando = caso != 1;
    dur = 3600;
    player_abrir(0, fonte); player_definir_episodio(1, 1);
    quadro(); quadro();
    pos = dur; terminou = 1;
    for (int i = 0; i < 120; i++) {
      quadro();
      assert(!player_quer_sair() && player_aberto());
      assert(!posplay_pediu_episodio(&t, &e));
    }

    // The delayed desc_episodios response publishes the next episode.
    cat_definir_episodios(0, eps, 2); carregando = 0;
    for (int i = 0; i < 600; i++) quadro();
    assert(!player_quer_sair() && player_aberto());
    assert(posplay_pediu_episodio(&t, &e) && t == 1 && e == 2);
    assert(!posplay_pediu_episodio(NULL, NULL));
    player_encerrar();
  }
  puts("PASS: series EOF waits for the episode list and preserves autoplay");
  SDL_Quit();
  return 0;
}
