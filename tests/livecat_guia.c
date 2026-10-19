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
int main(void) {
  M3uCtx ctx = {0};
  dados_iniciar(NULL);
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
  stagingZerar(); sTeto = 100;
  assert(cacheFonteLer(source) == 4);
  sugerirSecoes(); selecionarSecoes();
  assert(sNCanais == 1 && !strcmp(sCanais[0].nome, "Lumen"));
  assert(posts == 1);
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
  return 0;
}
