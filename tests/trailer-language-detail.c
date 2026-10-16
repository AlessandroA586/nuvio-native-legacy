/* Production detail caller, real TMDB parsing, deterministic player/network edges. */
#include "../src/detail.c"
#include <assert.h>
void extras_teste_trailer_audio(const char *);
int extras_teste_trailers_ficha(const char *, int, const char *, char [][16], int);
static CatItem fixture;
static int externalOpens, nativeOpens;
const CatItem *cat_item(int i) { (void)i; return &fixture; }
int ajustes_trailer_fonte(void) { return TRF_AUTO; }
int ajustes_trailer_som(void) { return 1; }
int trailer_suportado(void) { return 1; }
void trailer_abrir(const char *u, GfxRect r, int s, int c) {
  (void)u; (void)r; (void)s; (void)c; nativeOpens++;
}
void extras_trailer_abrir(int i) {
  assert(!strcmp(extras_trailer_yt(i), "trailerBR01")); externalOpens++;
}
const char *trailerapple_url(const char *id) { (void)id; return "https://apple/english.m3u8"; }
int trailerapple_respondeu(const char *id) { (void)id; return 1; }
const char *trailerimdb_url(const char *id, const char **n) { (void)id; (void)n; return "https://imdb/english.mp4"; }
int trailerimdb_respondeu(const char *id) { (void)id; return 1; }
int main(void) {
  char json[8192], yt[EX_TRAILER_MAX][16];
  FILE *f = fopen("tests/fixtures/tmdb_movie_videos_dub.json", "r"); assert(f);
  json[fread(json, 1, sizeof json - 1, f)] = 0; fclose(f);
  strcpy(fixture.imdb, "tt205");
  extras_teste_trailer_audio("pt");
  extras_teste_trailers_ficha(json, 0, "en-US", yt, EX_TRAILER_MAX);
  tocarTrailerCheio(0);
  assert(externalOpens == 1 && nativeOpens == 0);
  extras_teste_trailer_audio("~");
  extras_teste_trailers_ficha(json, 0, "en-US", yt, EX_TRAILER_MAX);
  tocarTrailerCheio(0);
  assert(externalOpens == 1 && nativeOpens == 1);
  puts("ok real native detail: Portuguese opens dubbed trailer externally; Original uses native English");
  return 0;
}
