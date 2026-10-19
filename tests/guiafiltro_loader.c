// Exercise the real staging, Xtream/M3U callbacks, cache and fallback paths.
#include "guiafiltro.h"
#include <errno.h>
static char *filtroResposta(const char *url);
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
void desc_remontar_fileiras(void) {}
void desc_repetir(void) {}
int player_aberto(void) { return 0; }
int segurar_venceu(Uint32 now, Uint32 since, const char *screen) {
  (void)now; (void)since; (void)screen; return 0;
}
void teclado_atualizar(float dt, Uint32 now) { (void)dt; (void)now; }
int teclado_resultado(void) { return 0; }
const char *teclado_texto(void) { return ""; }
int xtream_epg_curto(const char *id, XtreamProg *out, int cap, int *status) {
  (void)id; (void)out; (void)cap; *status = 0; return 0;
}
int xtream_ultimo_retry_after(void) { return 0; }
char *rede_baixar_bin_medido_controle(const char *url, int seconds, const char *const *headers,
                                     const RedeControle *control, long *size, RedeMedida *measure) {
  (void)url; (void)seconds; (void)headers; (void)control; (void)measure; *size = 0; return NULL;
}
static int pageRequests, pageScenario, sourceScenario, sourceRequests;
static int discoveryRequests, loadRequests;
int cat_n_fileiras(void) { discoveryRequests++; return 0; }
const CatFileira *cat_fileira(int i) { (void)i; return NULL; }
// Keep all test work foreground; record load launches without creating threads.
int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *u) {
  (void)t; (void)a; (void)fn; (void)u; loadRequests++; return EAGAIN;
}
static char *filtroResposta(const char *url) {
  char body[24000];
  size_t used = 0;
  if (sourceScenario) {
    sourceRequests++;
    if (sourceScenario == 3) return NULL;
    if (!strcmp(url, "xtream")) return strdup(sourceScenario == 1 ? "X 4K" : "X HD");
    if (strstr(url, ".m3u")) return strdup(sourceScenario == 1 ?
      "#EXTM3U\n#EXTINF:-1 tvg-id=\"x\" group-title=\"Sports\",X 4K\nhttps://fixture.invalid/live\n" :
      "#EXTM3U\n#EXTINF:-1 tvg-id=\"x\" group-title=\"Sports\",X HD\nhttps://fixture.invalid/live\n");
    assert(strstr(url, "/catalog/tv/channels"));
    return strdup(sourceScenario == 1 ?
      "{\"metas\":[{\"id\":\"channel:1\",\"name\":\"X 4K\",\"genre\":[\"Sports\"]}]}" :
      "{\"metas\":[{\"id\":\"channel:1\",\"name\":\"X HD\",\"genre\":[\"Sports\"]}]}");
  }
  if (!pageScenario) return NULL;
  assert(strstr(url, "/catalog/tv/channels"));
  pageRequests++;
  used += (size_t)snprintf(body + used, sizeof body - used, "{\"metas\":[");
  int offset = pageScenario == 3 ? (pageRequests - 1) * 100 :
               pageScenario == 2 && pageRequests > 1 ? 100 : 0;
  int count = pageScenario == 2 && pageRequests > 1 ? 1 : 100;
  for (int i = 0; i < count; i++)
    used += (size_t)snprintf(body + used, sizeof body - used,
      "%s{\"id\":\"channel:%d\",\"name\":\"X %s\",\"genre\":[\"%s\"]}",
      i ? "," : "", offset + i, offset ? "4K" : "HD", i == 0 ? "Hidden" : "Sports");
  snprintf(body + used, sizeof body - used, "]}");
  return strdup(body);
}
static void paginationRegression(int mode) {
  memset(&fltLive, 0, sizeof fltLive); fltLive.modo = mode; sFltLive = fltLive;
  guiafiltro_cat_alternar(&sFltLive, "Hidden");
  sNFontes = nAddonsCarga = 1; nCargaUrls = 0;
  addonsCarga = calloc(1, sizeof *addonsCarga);
  strcpy(addonsCarga[0].base, "https://fixture.invalid"); addonsCarga[0].ativo = 1;
  memset(sFontes, 0, sizeof sFontes);
  strcpy(sFontes[0].base, addonsCarga[0].base);
  strcpy(sFontes[0].id, "channels"); strcpy(sFontes[0].tipo, "tv");
  pageRequests = 0; pageScenario = 1; cargaCancelada = 0;
  fioGuia(NULL);
  printf("pagination mode=%d requests=%d (expected 2)\n", mode, pageRequests); fflush(stdout);
  assert(pageRequests == 2);
  // A full filtered page must not hide a later, distinct 4K page.
  if (mode == GF_SO_4K) {
    sFltLive.nCat = 0;
    nAddonsCarga = 1; addonsCarga = calloc(1, sizeof *addonsCarga);
    strcpy(addonsCarga[0].base, sFontes[0].base); addonsCarga[0].ativo = 1;
    pageRequests = 0; pageScenario = 2;
    fioGuia(NULL);
    assert(pageRequests == 2 && sNCanais == 1 && !strcmp(sCanais[0].nome, "X 4K"));
  }
  pageScenario = 0;
}
static void capRegression(void) {
  paginationRegression(GF_TODAS);
  memset(&sFltLive, 0, sizeof sFltLive);
  tetoCarga = 900;
  nAddonsCarga = 1; addonsCarga = calloc(1, sizeof *addonsCarga);
  strcpy(addonsCarga[0].base, sFontes[0].base); addonsCarga[0].ativo = 1;
  pageRequests = 0; pageScenario = 3;
  fioGuia(NULL);
  printf("cap requests=%d (expected 9)\n", pageRequests); fflush(stdout);
  assert(pageRequests == 9 && sNCanais == 900);
}
static void cacheRefreshRegression(void) {
  for (int mode = GF_SO_4K; mode < GF_MODOS; mode++) {
    int fl = begin(mode, 5);
    guiafiltro_cat_alternar(&sFltLive, "Hidden");
    guiafiltro_canal_alternar(&sFltLive, "xtream:4", "Z 4K", "Sports");
    channel(fl, "Sports", "xtream:1", "X HD");
    channel(fl, "Sports", "xtream:2", "X 4K");
    channel(fl, "Hidden", "xtream:3", "Y HD");
    channel(fl, "Sports", "xtream:4", "Z 4K");
    adotarStaging(); cacheGravar();
    begin(GF_TODAS, 10);
    int restored = cacheFonteLer("xt");
    printf("cache mode=%d restored=%d (expected 4)\n", mode, restored); fflush(stdout);
    assert(restored == 4);
    assert(!strcmp(sCanais[0].nome, "X HD") && !strcmp(sCanais[3].nome, "Z 4K"));
    // Same selected ID, changed raw metadata: publication must still refresh disk.
    fl = begin(mode, 5); fltLive = sFltLive;
    channel(fl, "Sports", "xtream:2", "X 4K"); publicar();
    fl = begin(mode, 5);
    channel(fl, "Sports", "xtream:2", "X 4K");
    channel(fl, "Sports", "xtream:1", "X SD"); publicar();
    begin(GF_TODAS, 10); assert(cacheFonteLer("xt") == 2);
    assert(!strcmp(sCanais[1].nome, "X SD"));
    // A successful source filtered to zero must also refresh its raw cache.
    fl = begin(GF_SO_4K, 5); fltLive = sFltLive;
    channel(fl, "Sports", "xtream:5", "New SD"); publicar();
    begin(GF_TODAS, 10); assert(cacheFonteLer("xt") == 1);
    assert(!strcmp(sCanais[0].nome, "New SD"));
  }
}
static void filteredEmptyRegression(int type, int hidden, int retry) {
  const char *ref = type == LTV_M3U ? "https://fixture.invalid/list.m3u" :
                    type == LTV_ADDON ? "https://fixture.invalid" : "";
  char key[LTV_CHAVE];
  ltv_chave_fonte(type, ref, key, sizeof key);
  xtOk = type == LTV_XTREAM;
  nCargaUrls = type == LTV_M3U;
  if (nCargaUrls) {
    cargaUrls[0].tipo = LTV_URL_M3U;
    strcpy(cargaUrls[0].url, ref); strcpy(cargaUrls[0].nome, "Fixture");
  }
  nAddons = type == LTV_ADDON;
  bases[0] = ref;
  memset(sFontes, 0, sizeof sFontes);
  if (nAddons) {
    strcpy(sFontes[0].base, ref); strcpy(sFontes[0].tipo, "tv");
    strcpy(sFontes[0].id, "channels");
  }
  // Seed stale 4K cache through the actual provider load and publication.
  for (int pass = 1; pass <= 2; pass++) {
    memset(&fltLive, 0, sizeof fltLive);
    if (pass == 2) {
      if (hidden) guiafiltro_cat_alternar(&fltLive, "Sports");
      else fltLive.modo = GF_SO_4K;
    }
    sFltLive = fltLive; sourceScenario = retry ? 2 : pass;
    sNFontes = nAddons;
    nAddonsCarga = nAddons;
    if (nAddons) {
      addonsCarga = calloc(1, sizeof *addonsCarga);
      strcpy(addonsCarga[0].base, ref); addonsCarga[0].ativo = 1;
    }
    fioGuia(NULL);
    if (pass == 2 && !retry) {
      assert(sNCanais == 0 && !sLtvF[0].deCache && sLtvF[0].saude == LTV_SAUDE_OK);
    }
    drenarCarga();
    assert(nCanais == (pass == 1 ? 1 : 0));
  }
  if (retry) {
    aberta = 1; previewLigado = 0; ultTentativa = 1;
    int requests = sourceRequests;
    for (Uint32 now = 11002; now < 50000; now += 11001) guia_atualizar(0.016f, now);
    assert(loadRequests == 0 && discoveryRequests == 0 && sourceRequests == requests);
    // Explicit refresh and configuration changes still launch loads.
    guia_limpar_lista(); assert(loadRequests == 1);
    guia_fontes_mudaram(); assert(loadRequests == 2);
    estado = G_FALHOU; ultTentativa = 1;
    guia_atualizar(0.016f, 60000);
    assert(loadRequests == 3 && discoveryRequests > 0);
  } else {
    // Fresh raw HD replaces stale 4K on disk; failure still uses that cache.
    memset(&sFltLive, 0, sizeof sFltLive);
    stagingZerar(); sTeto = 10;
    assert(cacheFonteLer(key) == 1 && !strcmp(sCanais[0].nome, "X HD"));
    sourceScenario = 3; sNFontes = nAddons;
    fioGuia(NULL);
    assert(sNCanais == 1 && sLtvF[0].deCache && sLtvF[0].saude == LTV_SAUDE_DEGRADADA);
  }
  sourceScenario = 0;
}
static void visibilityRegression(void) {
  int fl = begin(GF_PREFERIR_4K, 5);
  channel(fl, "Sports", "xtream:1", "X 4K");
  channel(fl, "News", "xtream:2", "Y 4K");
  channel(fl, "News", "xtream:3", "Z 4K");
  adotarStaging(); cacheGravar();
  char name[64]; cacheNomeFonte("xt", name, sizeof name);
  char *cached = dados_ler(name); free(cached);
  filtrosAbrir();
  guiafiltro_cat_alternar(&fltLive, "Sports");
  guiafiltro_canal_alternar(&fltLive, "xtream:2", "Y 4K", "News");
  fSujo = 1; fioVivo = 1; // Exercise cancellation without starting a thread.
  painelFechar(); fioVivo = 0;
  assert(nCanais == 1 && !strcmp(canais[0].id, "xtream:3"));
  assert(nCats == 1 && nZapL == 1 && ltvF[0].nCanais == 1);
  sNFontes = nAddonsCarga = nCargaUrls = 0; cargaCancelada = 0;
  sFltLive = fltLive; fioGuia(NULL); publicar();
  assert(nCanais == 1 && !strcmp(canais[0].id, "xtream:3"));
  filtrosAbrir(); guiafiltro_cat_alternar(&fltLive, "News");
  fSujo = 1; fioVivo = 1; painelFechar(); fioVivo = 0;
  assert(nCanais == 0 && nCats == 0 && nZapL == 0);
}
static void variantsRegression(void) {
  int fl = begin(GF_PREFERIR_4K, 5);
  channel(fl, "Sports", "xtream:1", "X SD");
  channel(fl, "Sports", "xtream:2", "X HD");
  channel(fl, "Sports", "xtream:3", "X FHD");
  channel(fl, "Sports", "xtream:4", "X 4K");
  adotarStaging();
  GuiaVariante v[5];
  assert(guia_variantes("xtream:4", v, 3) == 3);
  assert(!strcmp(v[0].nome, "X 4K") && !strcmp(v[1].nome, "X FHD") && !strcmp(v[2].nome, "X HD"));
  assert(guia_variantes("xtream:4", v, 1) == 1 && !strcmp(v[0].nome, "X 4K"));
  guiafiltro_canal_alternar(&fltLive, "xtream:3", "X FHD", "Sports");
  assert(guia_variantes("xtream:4", v, 3) == 3);
  assert(!strcmp(v[1].nome, "X HD") && !strcmp(v[2].nome, "X SD"));
}
static void categoriesRegression(void) {
  int fl = begin(GF_PREFERIR_4K, 1);
  channel(fl, "Sports", "xtream:1", "X FHD");
  channel(fl, "UHD", "xtream:2", "X 4K");
  assert(sNCanais == 1 && !strcmp(sCanais[0].id, "xtream:2"));
  adotarStaging();
  GuiaVariante v[3];
  assert(guia_variantes("xtream:2", v, 3) == 2);
  fl = begin(GF_TODAS, 3);
  channel(fl, "Sports", "xtream:1", "X FHD");
  channel(fl, "UHD", "xtream:2", "X 4K");
  adotarStaging();
  assert(guia_variantes("xtream:2", v, 3) == 2);
}
static void inventoryRegression(void) {
  for (int mode = GF_TODAS; mode < GF_MODOS; mode++) {
    int fl = begin(mode, 5);
    if (mode == GF_TODAS) guiafiltro_cat_alternar(&sFltLive, "Sports");
    channel(fl, "Sports", "xtream:1", "X FHD");
    channel(fl, "Sports", "xtream:2", "X 4K");
    adotarStaging(); cacheGravar();
    filtrosAbrir(); fMontarCanais("Sports");
    assert(fN == 2);
    assert(!strcmp(fIt[0].id, "xtream:1") && !strcmp(fIt[1].id, "xtream:2"));
    // Offline inventory must remain available after staging reset.
    begin(mode, 5); cacheFonteLer("xt"); adotarStaging();
    filtrosAbrir(); fMontarCanais("Sports"); assert(fN == 2);
  }
}
static void lateInventoryRegression(void) {
  for (int scenario = 0; scenario < 3; scenario++) {
    int fl = begin(scenario == 1 ? GF_TODAS : GF_SO_4K, 5);
    if (scenario == 1) guiafiltro_cat_alternar(&sFltLive, "Hidden");
    for (int i = 0; i < 4000; i++) {
      char id[32]; snprintf(id, sizeof id, "xtream:%d", i + 1);
      channel(fl, scenario == 1 ? "Hidden" : scenario == 2 ? "UHD" : "Sports", id, "X HD");
    }
    channel(fl, "UHD", "xtream:4001", "Late 4K");
    channel(fl, "UHD", "xtream:4002", "Later 4K");
    channel(fl, "UHD", "xtream:4003", "Late HD");
    fltLive = sFltLive; adotarStaging(); cacheGravar();
    for (int offline = 0; offline < 2; offline++) {
      if (offline) { stagingZerar(); sFltLive = fltLive; sTeto = 5; cacheFonteLer("xt"); adotarStaging(); }
      assert(nCanais == (scenario == 1 ? 3 : 2));
      filtrosAbrir(); fMontarCanais("UHD");
      int found = 0;
      for (int i = 0; i < fN; i++) if (!strcmp(fIt[i].id, "xtream:4001")) found++;
      assert(found == 1);
      int later = 0;
      for (int i = 0; i < fN; i++) if (!strcmp(fIt[i].id, "xtream:4002")) later++;
      assert(later == 1);
      if (scenario != 2) {
        int raw = 0;
        for (int i = 0; i < fN; i++) if (!strcmp(fIt[i].id, "xtream:4003")) raw++;
        assert(raw == 1);
      }
      strcpy(fCatNome, "UHD"); fNivel = 1;
      for (paFoco = 0; paFoco < fN && strcmp(fIt[paFoco].id, "xtream:4001"); paFoco++) {}
      filtrosOk();
      assert(guiafiltro_canal_oculto(&fltLive, "xtream:4001"));
      filtrosOk();
    }
  }
}
static void replacementMetadataRegression(void) {
  for (int reverse = 0; reverse < 2; reverse++) {
    int fl = begin(GF_PREFERIR_4K, 1);
    for (int i = 0; i < 2; i++) {
      int uhd = reverse ? !i : i;
      XtreamItem it = {0}; XtCtx x = {0};
      x.fl = fl;
      it.id = uhd ? "xtream:2" : "xtream:1";
      it.nome = uhd ? "X 4K" : "X FHD";
      it.categoria = uhd ? "UHD" : "Sports";
      it.num = uhd ? 202 : 101; it.catOrdem = -1;
      sGrupoInfo(fl, it.categoria, 1);
      xtCanal(&it, &x);
    }
    assert(sNCanais == 1 && sRetidos == 1);
    adotarStaging(); cacheGravar();
    for (int offline = 0; offline < 2; offline++) {
      if (offline) { begin(GF_PREFERIR_4K, 1); cacheFonteLer("xt"); adotarStaging(); }
      char cat[64]; int number = 0;
      estado = G_PRONTO;
      assert(guia_info_canal("xtream:2", &number, NULL, cat, sizeof cat));
      assert(!strcmp(cat, "UHD") && number == 202);
      filtrosAbrir(); fMontarCanais("UHD");
      assert(fN == 1 && !strcmp(fIt[0].id, "xtream:2"));
      GuiaVariante v[3]; assert(guia_variantes("xtream:2", v, 3) == 2);
      guiafiltro_cat_alternar(&fltLive, "Sports"); empacotar();
      assert(nCanais == 1 && !strcmp(canais[0].id, "xtream:2"));
      guiafiltro_cat_alternar(&fltLive, "Sports");
    }
  }
  // Repeated replacements must not exhaust category or channel slots.
  int fl = begin(GF_PREFERIR_4K, GUIACORTE_MAX_CAT);
  for (int i = 0; i < GUIACORTE_MAX_CAT; i++) {
    char cat[32], id[32], name[32];
    snprintf(cat, sizeof cat, "Category %d", i);
    snprintf(id, sizeof id, "xtream:%d", i);
    snprintf(name, sizeof name, "Station %d FHD", i);
    channel(fl, cat, id, name);
  }
  channel(fl, "UHD", "xtream:999", "Station 0 4K");
  assert(sNCanais == GUIACORTE_MAX_CAT && sRetidos == GUIACORTE_MAX_CAT);
  assert(!strcmp(sCanais[0].id, "xtream:999") && !strcmp(sCats[sCanais[0].cat].nome, "UHD"));
}
static void replacementCategoryCapRegression(void) {
  int fl = begin(GF_PREFERIR_4K, GUIACORTE_MAX_CAT + 1);
  channel(fl, "Sports", "xtream:1", "X FHD");
  channel(fl, "Sports", "xtream:2", "Y FHD");
  for (int i = 1; i < GUIACORTE_MAX_CAT; i++) {
    char cat[32], id[32], name[32];
    snprintf(cat, sizeof cat, "Category %d", i);
    snprintf(id, sizeof id, "xtream:%d", i + 2);
    snprintf(name, sizeof name, "Station %d FHD", i);
    channel(fl, cat, id, name);
  }
  channel(fl, "UHD", "xtream:999", "X 4K");
  assert(!strcmp(sCanais[0].id, "xtream:999"));
  assert(sNCats == GUIACORTE_MAX_CAT);
  assert(sNCanais == GUIACORTE_MAX_CAT + 1 && sRetidos == sNCanais);
  assert(sCanais[0].cat == sCanais[1].cat);
  assert(!strcmp(sCats[sCanais[1].cat].nome, "Sports"));
  // A later lower-quality version must remain a fallback, never replace 4K.
  channel(fl, "UHD", "xtream:1000", "X HD");
  assert(!strcmp(sCanais[0].id, "xtream:999"));
  adotarStaging();
  GuiaVariante v[4];
  assert(guia_variantes("xtream:999", v, 4) == 3);
  assert(!strcmp(v[0].nome, "X 4K"));
  assert(!strcmp(v[1].nome, "X FHD"));
  assert(!strcmp(v[2].nome, "X HD"));
}
static void rawBudgetRegression(void) {
#ifdef GUIAFILTRO_RAW_TEST
  int fl = begin(GF_SO_4K, 1);
  for (int i = 0; i < 100000; i++) channel(fl, "Sports", "xtream:1", "X FHD");
  assert(sCache[fl] && ftell(sCache[fl]) <= 2 * 1024 * 1024);
  adotarStaging(); cacheGravar();
  assert(sCache[fl] == NULL);
#endif
}
int main(int argc, char **argv) {
  int fl;
  assert(argc == 2); dados_iniciar(argv[1]);
  fltPerfil = 1;
  const char *regression = getenv("GUIAFILTRO_CASE");
  if (regression) {
    if (!strncmp(regression, "pagination", 10))
      paginationRegression(!strcmp(regression, "pagination-4k") ? GF_SO_4K :
                           !strcmp(regression, "pagination-prefer") ? GF_PREFERIR_4K : GF_TODAS);
    else if (!strcmp(regression, "cap")) capRegression();
    else if (!strcmp(regression, "cache-refresh")) cacheRefreshRegression();
    else if (!strncmp(regression, "empty-", 6))
      filteredEmptyRegression(strstr(regression, "m3u") ? LTV_M3U :
                              strstr(regression, "addon") ? LTV_ADDON : LTV_XTREAM,
                              strstr(regression, "hidden") != NULL, strstr(regression, "retry") != NULL);
    else if (!strcmp(regression, "visibility")) visibilityRegression();
    else if (!strcmp(regression, "variants")) variantsRegression();
    else if (!strcmp(regression, "categories")) categoriesRegression();
    else if (!strcmp(regression, "inventory")) inventoryRegression();
    else if (!strcmp(regression, "late-inventory")) lateInventoryRegression();
    else if (!strcmp(regression, "replacement-metadata")) replacementMetadataRegression();
    else if (!strcmp(regression, "replacement-category-cap")) replacementCategoryCapRegression();
    else if (!strcmp(regression, "raw-budget")) rawBudgetRegression();
    else assert(0);
    printf("PASS: %s regression\n", regression);
    return 0;
  }
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
