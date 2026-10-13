// B6: trailer (PRONTO de 129 s sobre um titulo de 1 h 46) nao scrobbla ao
// vivo — nem start nem pause, Trakt ou Simkl. Titulo real continua falando.
#define main main_base
#include "player_visto_trailer.c"
#undef main
static int nScr;
int nv_cnt_scrobble(int ev, const char *id, double p, double d, double a, int x) {
  (void)ev; (void)id; (void)p; (void)d; (void)a; (void)x; nScr++; return 0;
}
static void ao_vivo(const char *imdb, double d) {
  CatItem c = {0};
  strcpy(c.titulo, "Fixture"); strcpy(c.imdb, imdb); strcpy(c.tipo, "movie");
  strcpy(c.meta, "1 h 46 min");
  cat_definir(&c, 1); dur = d; nScr = 0;
  player_abrir(0, NULL); player_definir_fonte(fonte); quadro();
  for (int i = 0; i < 60; i++) { agoraMono += 1; pos += 1; quadro(); }
  pausado = 1; quadro(); quadro();
  player_encerrar(); pausado = 0;
}
int main(void) {
  assert(SDL_Init(SDL_INIT_TIMER) == 0); dados_iniciar(""); ling_local_legenda("none");
  ao_vivo("tt22687801", 129.036);
  assert(nScr == 0);                 // trailer: nenhum start/pause
  ao_vivo("tt22687802", 6357.0);
  assert(nScr > 0);                  // controle: titulo real scrobbla
  puts("ok");
  return 0;
}
