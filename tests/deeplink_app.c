// Harness for the real deeplinkAbrir body, extracted by deeplink.sh.
#include "deeplink.h"
#include <assert.h>
#include <string.h>

typedef struct { long tmdb; const char *tipo; } CatItem;
static CatItem items[] = {{603, "channel"}, {603, "series"}, {603, "movie"}};
static int count, opened, played, requested, detail;
static int cwTocarT, cwTocarE, cwEspera;
enum { TELA_BUSCA, MENU_BUSCAR, TELA_LOGIN, TELA_ESCOLHA_PERFIL };
static int tela, busy;
static int desc_titulo_buscando(void) { return busy; }
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
  assert(id == 603 || id == 604);
  if (busy) return;
  requested = !strcmp(type, "tv") ? 1 : 2;
  busy = 1;
}

#include "deeplink_abrir.inc"

static void dispatch(void) {
#include "deeplink_dispatch.inc"
}

int main(int argc, char **argv) {
  (void)argv;
  if (argc > 1) {
    DeepLink pending;
    count = 0;
    deeplink_postar("nuvio-legacy://meta/movie/603");
    dispatch();
    assert(busy && requested == 2);
    deeplink_postar("nuvio-legacy://meta/series/604");
    dispatch();
    assert(deeplink_pegar(&pending) && pending.tmdb == 604);
    // Re-post B, then C: newest request survives until A finishes.
    deeplink_postar("nuvio-legacy://meta/series/604");
    deeplink_postar("nuvio-legacy://meta/series/603");
    dispatch();
    busy = 0;
    dispatch();
    assert(requested == 1 && busy);
    assert(!deeplink_pegar(&pending));
    return 0;
  }
  const char *urls[] = {"nuvio-legacy://play/movie/603",
                        "nuvio-legacy://play/series/603"};
  for (int series = 0; series <= 1; series++) {
    DeepLink d;
    assert(deeplink_parse(urls[series], &d));
    count = 3; opened = -1; played = requested = detail = busy = 0;
    deeplinkAbrir(&d);
    assert(opened == (series ? 1 : 2));
    assert(played == 1 && requested == 0);

    // Only the opposite media type (plus a channel) exists locally.
    items[1].tipo = series ? "movie" : "series";
    count = 2; opened = -1; played = requested = detail = busy = 0;
    deeplinkAbrir(&d);
    assert(opened == -1 && played == 0);
    assert(requested == (series ? 1 : 2));
    items[1].tipo = "series";
  }
  return 0;
}
