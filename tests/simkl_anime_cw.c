// Offline Simkl API-shaped fixtures exercise the actual CW merge.
#define main simkl_existing_main
#define trakt_enfeitar_lote fixture_enfeitar_lote
#define trakt_enfeitar_lote_sessao fixture_enfeitar_lote_sessao
#include "simkl_cw.c"
#undef main
#undef trakt_enfeitar_lote
#undef trakt_enfeitar_lote_sessao

#include "metaprov.h"
const char *metaprov_idioma(void) { return "en"; }
char *metaprov_meta_cinemeta_com(const char *t, const char *id, int s, MetaprovGet g, void *ctx) {
  (void)t; (void)id; (void)s; (void)g; (void)ctx; return NULL;
}
unsigned int SDL_GetTicks(void) { return 0; }
void marco(const char *m) { (void)m; }
char *rede_baixar(const char *u, int s) { (void)u; (void)s; return NULL; }
int cwo_conta_a_seguir(const char *id) { (void)id; return 0; }
void cwo_conta_trocar(const char *a, const char *b) { (void)a; (void)b; }
void cwo_marcar_estreia(const char *id, long long ms) { (void)id; (void)ms; }
int cwo_virada_aceita(long long e, long long a) { (void)e; (void)a; return 1; }
int ajustes_tmdb_cw(void) { return 0; }
const char *desc_chave_tmdb(void) { return ""; }
const char *desc_tmdb_idioma(void) { return "en"; }
char *metaprov_meta(const char *tipo, const char *id, int seg, int *prov) {
  (void)tipo; (void)id; (void)seg; (void)prov; return NULL;
}
void metaprov_duracao(char *s, size_t n) { (void)s; (void)n; }

char *rede_baixar_st_retry(const char *u, int s, const char *const *c, int *st, int *retry) {
  *retry = 0;
  return rede_baixar_st(u, s, c, st);
}
int rede_pedir(const RedePedido *p, RedeResposta *r) {
  (void)p; r->erro = REDE_INDISPONIVEL; return 0;
}
void rede_resposta_limpar(RedeResposta *r) { (void)r; }

int main(void) {
  static const char fixture[] =
    "{\"anime\":["
    "{\"last_watched\":\"E1122\",\"next_to_watch\":\"E1123\","
    "\"last_watched_at\":\"2026-01-22T21:00:00Z\",\"status\":\"watching\","
    "\"anime\":{\"title\":\"One Piece\",\"ids\":{\"simkl\":38636,\"mal\":21,\"anidb\":69,\"tmdb\":37854,\"imdb\":\"tt0388629\"}}},"
    "{\"last_watched\":\"S01E1122\",\"next_to_watch\":\"S01E1123\","
    "\"anime\":{\"title\":\"One Piece duplicate\",\"ids\":{\"imdb\":\"tt0388629\"}}},"
    "{\"last_watched\":\"E5\",\"next_to_watch\":\"E6\","
    "\"anime\":{\"title\":\"TMDB only\",\"ids\":{\"simkl\":8,\"tmdb\":123}}},"
    "{\"last_watched\":\"E9\",\"next_to_watch\":\"E10\","
    "\"anime\":{\"title\":\"Other TMDB\",\"ids\":{\"simkl\":9,\"tmdb\":456}}},"
    "{\"last_watched\":\"E1\",\"next_to_watch\":\"E2\","
    "\"anime\":{\"title\":\"Simkl only\",\"ids\":{\"simkl\":10,\"mal\":11,\"anidb\":12}}},"
    "{\"last_watched\":\"E1122\",\"next_to_watch\":null,"
    "\"anime\":{\"title\":\"Caught up\",\"ids\":{\"simkl\":11}}}]}";
  CatItem v[16];
  tokenAtual = TOKEN;
  PLAYBACK = "[]";
  WATCHING = "{\"shows\":[]}";
  WATCHING_ANIME = fixture;
  int n = simkl_continuar(v, 16);
  assert(n == 4);
  const char *expected[] = {"tt0388629:1:1123", "tmdb:123:1:6", "tmdb:456:1:10", "simkl:10:1:2"};
  for (int i = 0; i < n; i++) {
    assert(!strcmp(v[i].imdb, expected[i]));
    assert(!strcmp(v[i].tipo, "series") && v[i].progresso == 0);
    assert(simkl_e_a_seguir(v[i].imdb));
  }
  assert(v[0].temporada == 1 && v[0].episodio == 1123 && v[0].tmdb == 37854);
  assert(v[1].tmdb == 123 && v[2].tmdb == 456);
  proximoTeste = 0;
  assert(simkl_continuar(v, 16) == 0);
  puts("PASS: absolute anime next episode, fallback IDs, dedup and next-up setting");
  return 0;
}
