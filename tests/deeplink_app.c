// Harness for the real deeplinkAbrir body, extracted by deeplink.sh.
#include "deeplink.h"
#include <assert.h>
#include <string.h>

typedef struct { long tmdb; const char *tipo; } CatItem;
static CatItem items[] = {{603, "channel"}, {603, "series"}, {603, "movie"}};
static int count, opened, played, requested, detail;
static int cwTocarT, cwTocarE, cwEspera;
enum { TELA_BUSCA, MENU_BUSCAR };
static int player_aberto(void) { return 0; }
static int player_retido(void) { return 0; }
static void player_encerrar(void) {}
static int detail_aberto(void) { return detail; }
static void detail_fechar_seco(void) { detail = 0; }
static int spainel_aberto(void) { return 0; }
static void spainel_fechar(void) {}
static void trocarTela(int t) { (void)t; }
static void menu_definir_destino(int t) { (void)t; }
static void busca_definir_consulta(const char *s) { (void)s; }
static int cat_indice_por_imdb(const char *s) { (void)s; return -1; }
static int cat_n(void) { return count; }
static const CatItem *cat_item(int i) { return &items[i]; }
static void abrirPorIndice(int i) { opened = i; detail = 1; }
static void detail_pedir_reproduzir(void) { played++; }
static void desc_pedir_titulo(const char *s) { (void)s; assert(0); }
static void desc_pedir_titulo_tmdb(long id, const char *type) {
  assert(id == 603);
  requested = !strcmp(type, "tv") ? 1 : 2;
}

#include "deeplink_abrir.inc"

int main(void) {
  const char *urls[] = {"nuvio-legacy://play/movie/603",
                        "nuvio-legacy://play/series/603"};
  for (int series = 0; series <= 1; series++) {
    DeepLink d;
    assert(deeplink_parse(urls[series], &d));
    count = 3; opened = -1; played = requested = detail = 0;
    deeplinkAbrir(&d);
    assert(opened == (series ? 1 : 2));
    assert(played == 1 && requested == 0);

    // Only the opposite media type (plus a channel) exists locally.
    items[1].tipo = series ? "movie" : "series";
    count = 2; opened = -1; played = requested = detail = 0;
    deeplinkAbrir(&d);
    assert(opened == -1 && played == 0);
    assert(requested == (series ? 1 : 2));
    items[1].tipo = "series";
  }
  return 0;
}
