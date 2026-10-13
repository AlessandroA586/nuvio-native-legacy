// Real API fixtures: search must retain the Portuguese cut without an IMDb id.
// Reuse the discovery harness; only HTTP is replaced, catalog/parser are real.
#define main detalheanime_main
#include "detalheanime.c"
#undef main

static char *fixture(const char *name) {
  char path[256];
  snprintf(path, sizeof path, "tests/fixtures/farah/%s.json", name);
  FILE *f = fopen(path, "rb");
  assert(f);
  assert(!fseek(f, 0, SEEK_END));
  long n = ftell(f);
  assert(n >= 0 && !fseek(f, 0, SEEK_SET));
  char *body = malloc((size_t)n + 1);
  assert(body && fread(body, 1, (size_t)n, f) == (size_t)n);
  body[n] = 0;
  fclose(f);
  return body;
}

int main(void) {
  const char *queries[] = {"Meu Nome é Farah", "Meu Nome e Farah", "Adım Farah"};
  desc_alvos_busca_zerar();
  for (int q = 0; q < 3; q++) {
    char name[64];
    snprintf(name, sizeof name, "nuvio_search_%d", q);
    char *body = fixture(name);
    nRotas = 0; metaprov_zerar_pausa();
    rota("catalog.nuvio.tv", body);
    CatItem results[4];
    assert(consultarAlvo(&alvos[1], queries[q], results, 4) == 2);
    assert(!strcmp(results[0].imdb, "tt21874046"));
    assert(!strcmp(results[1].imdb, "tmdb:335918"));
    assert(!strcmp(results[1].titulo, "Meu Nome é Farah"));
    free(body);
  }
  nRotas = 0;
  puts("farah: search retains IMDb and TMDB versions, accented and Turkish titles");
  return 0;
}
