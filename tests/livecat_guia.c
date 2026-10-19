// Exercise real guide staging and regrouping with a mocked Worker.
#define NV_PERFIL_SESSAO_FORTE
#include "../src/guia.c"
#include <assert.h>

static int enabled = 1, posts;
int ajustes_livetv_sugerir(void) { return enabled; }
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t g) { return g == 0; }
int perfis_sessao_travar(uint64_t g) { return g == 0; }
void perfis_sessao_destravar(void) {}
const char *i18n(const char *s) { return s; }
int perfis_ativo(void) { return 1; }
char *rede_postar_st(const char *u, int s, const char *const *h, const char *b, int *st) {
  (void)u; (void)s; (void)h; (void)b; (void)st; abort();
}
static char *worker(const char *url, const char *body, int *status) {
  assert(!strcmp(url, "http://rec.test/livetv/categorias"));
  assert(strstr(body, "aurora") && strstr(body, "lumen"));
  assert(!strstr(body, "espn") && !strstr(body, "classified"));
  posts++; *status = 200;
  return strdup("{\"cats\":{\"aurora\":\"Esportes\",\"lumen\":\"Infantil\"}}");
}
static char *coldWorker(const char *url, const char *body, int *status) {
  assert(strstr(body, "velora") && strstr(body, "nimbo") && strstr(body, "talume"));
  (void)url; posts++; *status = 200;
  return strdup("{\"cats\":{\"velora\":\"Esportes\",\"nimbo\":\"Infantil\",\"talume\":\"Infantil\"}}");
}
static void addM3u(M3uCtx *ctx, const char *name, const char *group, const char *id) {
  LtvM3uCanal c = {0}; c.nome = name; c.grupo = group; c.url = id;
  c.tvgId = c.tvgNome = c.logo = c.ua = c.referer = "";
  assert(m3uCanal(&c, ctx));
}
static void checkGroups(void) {
  GFonteLtv *f = &sLtvF[0];
  int sports = 0, kids = 0, other = 0;
  for (int i = 0; i < f->ngi; i++) {
    if (!strcmp(f->gi[i].nome, "Esportes")) sports = f->gi[i].n;
    if (!strcmp(f->gi[i].nome, "Infantil")) kids = f->gi[i].n;
    if (!strcmp(f->gi[i].nome, "Outros")) other = f->gi[i].n;
  }
  assert(sports == 2 && kids == 1 && other == 0);
}
int main(int argc, char **argv) {
  M3uCtx ctx = {0};
  dados_iniciar(NULL);
  (void)argv;
  if (argc > 1) {
    char source[LTV_CHAVE];
    const char *selected[] = {"Infantil"};
    ltv_chave_fonte(LTV_M3U, "https://private/list?token=secret", source, sizeof source);
    ltv_pref_definir_grupos(ltv_pref(source, 1), selected, 1);
    sTeto = 1;
    assert(cacheFonteLer(source) == 1);
    selecionarSecoes();
    assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "Lumen"));
    assert(!strcmp(sCats[sCanais[0].cat].nome, "Infantil") && posts == 0);
    puts("PASS: cold process restores selected suggested section from disk without network");
    return 0;
  }
  sTeto = 100;
  ctx.fl = sFonte(LTV_M3U, "https://private/list?token=secret", "My list", "");
  addM3u(&ctx, "Aurora HD", "", "https://private/1");
  addM3u(&ctx, "Lumen", "", "https://private/2");
  addM3u(&ctx, "ESPN HD", "", "https://private/3");
  addM3u(&ctx, "Already Classified", "Provider custom", "https://private/4");
  assert(sNCanais == 4);
  assert(sCanais[0].sug && sCanais[1].sug && !sCanais[2].sug && !sCanais[3].sug);
  livecat_postar_teste(worker);
  enabled = 0; sugerirSecoes(); assert(posts == 0);
  enabled = 1; sugerirSecoes();
  assert(posts == 1);
  assert(!strcmp(sCats[sCanais[0].cat].nome, "Esportes"));
  assert(!strcmp(sCats[sCanais[1].cat].nome, "Infantil"));
  assert(!strcmp(sCats[sCanais[2].cat].nome, "Esportes"));
  assert(!strcmp(sCats[sCanais[3].cat].nome, "Provider custom"));
  checkGroups();
  puts("PASS: suggested sections appear in source selector with accurate counts");
  puts("PASS: unclassified list builds suggested sections; local/provider categories untouched; setting off sends nothing");
  cargaCancelada = 1; sugerirSecoes(); assert(posts == 1);
  puts("PASS: cancelled guide load sends nothing");
  cargaCancelada = 0;

  // Persist eligibility and original groups, not the suggested section.
  char source[LTV_CHAVE]; snprintf(source, sizeof source, "%s", sLtvF[0].chave);
  canais = sCanais; cats = sCats; nCanais = sNCanais;
  ltvF[0] = sLtvF[0]; cacheFonteGravar(0);
  canais = NULL; cats = NULL; nCanais = 0; memset(ltvF, 0, sizeof ltvF);
  stagingZerar(); sTeto = 100;
  assert(cacheFonteLer(source) == 4);
  assert(sCanais[0].sug && !strcmp(sCats[sCanais[0].cat].nome, "Outros"));
  enabled = 0; sugerirSecoes();
  assert(!strcmp(sCats[sCanais[0].cat].nome, "Outros"));
  enabled = 1; sugerirSecoes();
  assert(posts == 1 && !strcmp(sCats[sCanais[0].cat].nome, "Esportes"));
  assert(!strcmp(sCats[sCanais[3].cat].nome, "Provider custom"));
  checkGroups();
  // Choosing one suggested section must survive a fresh provider load.
  LtvPref *pref = ltv_pref(source, 1);
  const char *selected[] = {"Infantil"};
  ltv_pref_definir_grupos(pref, selected, 1);
  stagingZerar(); sTeto = 100;
  ctx.fl = sFonte(LTV_M3U, "https://private/list?token=secret", "My list", "");
  ctx.nsel = sGruposEscolhidos(ctx.fl, ctx.sel, 256);
  addM3u(&ctx, "Aurora HD", "", "https://private/1");
  addM3u(&ctx, "Lumen", "", "https://private/2");
  addM3u(&ctx, "ESPN HD", "", "https://private/3");
  sugerirSecoes();
  selecionarSecoes();
  assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "Lumen"));
  assert(!strcmp(sCats[sCanais[0].cat].nome, "Infantil"));
  assert(posts == 1);
  // Startup adopts the source cache without running the network suggestion pass.
  int failures = 0;
  stagingZerar(); sTeto = 100;
  assert(cacheFonteLer(source) > 0);
  selecionarSecoes();
  if (sNCanais != 1 || strcmp(sCanais[0].nome, "Lumen") ||
      strcmp(sCats[sCanais[0].cat].nome, "Infantil") || posts != 1) {
    puts("FAIL: cold source cache retains the selected suggested section without network");
    failures++;
  } else puts("PASS: cold source cache retains the selected suggested section without network");

  // An earlier unselected candidate must not spend the last retained-channel slot.
  stagingZerar(); sTeto = 1;
  ctx.fl = sFonte(LTV_M3U, "https://private/list?token=secret", "My list", "");
  ctx.nsel = sGruposEscolhidos(ctx.fl, ctx.sel, 256);
  addM3u(&ctx, "Aurora HD", "", "https://private/1");
  addM3u(&ctx, "Lumen", "", "https://private/2");
  sugerirSecoes(); selecionarSecoes();
  if (sNCanais != 1 || strcmp(sCanais[0].nome, "Lumen") ||
      sRetidos != 1 || posts != 1) {
    puts("FAIL: selected suggested section gets capacity after an unselected M3U candidate");
    failures++;
  } else puts("PASS: selected suggested section gets capacity after an unselected M3U candidate");

  stagingZerar(); sTeto = 1;
  cacheFonteLer(source);
  selecionarSecoes();
  if (sNCanais != 1 || strcmp(sCanais[0].nome, "Lumen") ||
      sRetidos != 1 || posts != 1) {
    puts("FAIL: cold source cache applies suggested selection before channel capacity");
    failures++;
  } else puts("PASS: cold source cache applies suggested selection before channel capacity");
  ltv_pref_definir_grupos(pref, NULL, 0);
  stagingZerar(); sTeto = 100;
  assert(cacheFonteLer(source) == 4);
  sugerirSecoes();
  puts("PASS: source cache preserves fallback; suggested selector selection survives reload without network");

  XtCtx xt = {0}; xt.fl = sFonte(LTV_XTREAM, "", "Xtream", ""); xt.decidido = 1;
  XtreamItem it = {0};
  it.id = "xtream:1"; it.nome = "Aurora HD"; it.categoria = "Outros";
  it.logo = it.epgId = ""; it.semCategoria = 1;
  assert(xtCanal(&it, &xt) && sCanais[4].sug);
  it.id = "xtream:2"; it.semCategoria = 0;
  assert(xtCanal(&it, &xt) && !sCanais[5].sug);
  sugerirSecoes();
  assert(!strcmp(sCats[sCanais[4].cat].nome, "Esportes"));
  assert(!strcmp(sCats[sCanais[5].cat].nome, "Outros"));
  puts("PASS: uncategorized Xtream channels get suggestions; provider category named Outros stays untouched");

  stagingZerar(); sTeto = 1;
  ltv_pref_definir_grupos(pref, selected, 1);
  ctx.fl = sFonte(LTV_M3U, "https://private/list?token=secret", "My list", "");
  ctx.nsel = sGruposEscolhidos(ctx.fl, ctx.sel, 256);
  livecat_postar_teste(coldWorker);
  addM3u(&ctx, "Velora", "", "https://private/5");
  addM3u(&ctx, "Nimbo", "", "https://private/6");
  addM3u(&ctx, "Talume", "", "https://private/7");
  sugerirSecoes(); selecionarSecoes();
  assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "Nimbo") && sRetidos == 1);
  assert(posts == 2 && sLtvF[ctx.fl].cortada);
  puts("PASS: uncached suggestions filter before quota and retain at most the channel cap");
  return failures ? 1 : 0;
}
