// Exercise the real staging, Xtream/M3U callbacks, cache and fallback paths.
#include "guiafiltro.h"
#ifdef GUIAFILTRO_BASELINE
static int baseline_avaliar(const GuiaFiltro *f, GuiaFiltroDup *d, int fonte,
    const char *grupo, const char *id, const char *nome, const char *origem, int *idx) {
  (void)f; (void)d; (void)fonte; (void)grupo; (void)id; (void)nome; (void)origem; (void)idx;
  return GF_ENTRA;
}
#define guiafiltro_avaliar baseline_avaliar
#endif
#define GUIAFILTRO_LOADER
#define main cache_test_main
#include "guiacache.c"
#undef main

static int begin(int modo, int teto) {
  memset(&fltLive, 0, sizeof fltLive);
  fltLive.modo = modo;
  sFltLive = fltLive;
  stagingZerar(); sTeto = teto;
  return sFonte(LTV_XTREAM, "", "Fixture", "");
}
static void channel(int fl, const char *cat, const char *id, const char *nome) {
  GCru c = {0};
  c.id = id; c.nome = nome; c.grupo = cat;
  c.logo = c.desc = c.epgId = c.tvgNome = c.url = c.ua = c.ref = c.tipo = "";
  sGrupoInfo(fl, cat, 1);
  sCanalAdd(fl, &c);
}
int main(int argc, char **argv) {
  int fl;
  assert(argc == 2); dados_iniciar(argv[1]);
  fltPerfil = 1;
  for (int modo = 0; modo < GF_MODOS; modo++) {
    fl = begin(modo, 5);
    channel(fl, "Sports", "xtream:1", "X 4K");
    channel(fl, "Sports", "xtream:2", "X FHD");
    channel(fl, "Sports", "xtream:3", "X HD");
    channel(fl, "Sports", "xtream:4", "Y FHD");
    channel(fl, "Sports", "xtream:5", "Y SD");
    assert(sNCanais == (modo == GF_TODAS ? 5 : modo == GF_SO_4K ? 1 : 2));
    if (modo) assert(!strcmp(sCanais[0].nome, "X 4K"));
    if (modo == GF_PREFERIR_4K) assert(!strcmp(sCanais[1].nome, "Y FHD"));
    adotarStaging();
    if (modo == GF_PREFERIR_4K) {
      GuiaVariante v[5];
      assert(guia_variantes("xtream:1", v, 5) == 3);
      assert(!strcmp(v[0].nome, "X 4K") && !strcmp(v[1].nome, "X FHD") && !strcmp(v[2].nome, "X HD"));
    }
  }
  // Hidden categories consume neither category nor channel slots.
  fl = begin(GF_TODAS, 2);
  guiafiltro_cat_alternar(&sFltLive, "Hidden");
  for (int i = 0; i < GUIACORTE_MAX_CAT + 1; i++) channel(fl, "Hidden", "hidden", "Hidden HD");
  channel(fl, "Visible", "1", "Visible HD");
  channel(fl, "Visible", "2", "Visible FHD");
  assert(sNCanais == 2 && sNCats == 1 && sRetidos == 2);
  // A later best variant replaces its slot even when the cap is full.
  fl = begin(GF_PREFERIR_4K, 1);
  channel(fl, "Sports", "1", "X SD"); channel(fl, "Sports", "2", "X UHD HEVC");
  assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "X UHD HEVC"));
  // Xtream must not reject a large raw group before quality filtering.
  { XtCtx x = {0}; XtreamItem c = {0};
    fl = begin(GF_SO_4K, 1); x.fl = fl;
    xtCat("Sports", 0, 10000, &x);
    c.id = "1"; c.nome = "X SD"; c.categoria = "Sports"; c.catOrdem = 0;
    xtCanal(&c, &x); c.id = "2"; c.nome = "X 4K"; xtCanal(&c, &x);
    assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "X 4K")); free(x.giDe);
  }
  // M3U may improve an admitted channel after reaching the cap.
  { M3uCtx x = {0}; LtvM3uCanal c = {0};
    fl = begin(GF_PREFERIR_4K, 1); x.fl = fl;
    c.grupo = "Sports"; c.nome = "X SD"; c.tvgId = "x"; c.url = "https://fixture.invalid/sd";
    c.logo = c.tvgNome = c.ua = c.referer = "";
    m3uCanal(&c, &x); c.nome = "X 4K"; m3uCanal(&c, &x);
    assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "X 4K"));
  }
  // Keep the unfiltered cache and reconstruct fallbacks on offline reload.
  fl = begin(GF_TODAS, 5);
  channel(fl, "Sports", "xtream:1", "X HD"); channel(fl, "Sports", "xtream:2", "X 4K");
  adotarStaging(); cacheGravar();
  begin(GF_PREFERIR_4K, 5); assert(cacheFonteLer("xt") == 1); adotarStaging();
  { GuiaVariante v[3]; assert(guia_variantes("xtream:2", v, 3) == 2); }
  cacheGravar();
  begin(GF_TODAS, 5); assert(cacheFonteLer("xt") == 2);
  // Profile persistence round-trip through the real data directory.
  memset(&fltLive, 0, sizeof fltLive); fltLive.modo = GF_PREFERIR_4K;
  guiafiltro_cat_alternar(&fltLive, "Hidden"); fltGravar();
  fltPerfil = -1; fltGarantir();
  assert(fltLive.modo == GF_PREFERIR_4K && guiafiltro_cat_oculta(&fltLive, "Hidden"));
  // Switching profile reloads its own filter instead of leaking the old one.
  perfilAtivo = 2; fltPerfil = -1; fltGarantir();
  assert(fltLive.modo == GF_TODAS && !guiafiltro_cat_oculta(&fltLive, "Hidden"));
  perfilAtivo = 1; fltPerfil = -1; fltGarantir();
  assert(fltLive.modo == GF_PREFERIR_4K && guiafiltro_cat_oculta(&fltLive, "Hidden"));
  // Filtering everything must publish an empty grid, not retain old channels.
  adotarStaging();
  fl = begin(GF_SO_4K, 1); channel(fl, "Sports", "1", "X SD");
  publicar(); assert(nCanais == 0);
  // Real platform caps remain unchanged.
  fl = begin(GF_TODAS, GUIACORTE_MAX_CANAL + 5);
  for (int i = 0; i < GUIACORTE_MAX_CANAL + 5; i++) channel(fl, "Sports", "1", "X");
  assert(sNCanais == GUIACORTE_MAX_CANAL);
  fl = begin(GF_TODAS, GUIACORTE_MAX_CANAL);
  for (int i = 0; i < GUIACORTE_MAX_CAT + 5; i++) {
    char cat[32]; snprintf(cat, sizeof cat, "Category %d", i); channel(fl, cat, "1", "X");
  }
  assert(sNCats == GUIACORTE_MAX_CAT && sNCanais == GUIACORTE_MAX_CAT);
  puts("PASS: real Live TV loader, source callbacks, empty results and caps");
  return 0;
}
