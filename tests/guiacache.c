// CACHE POR FONTE do guia (M1): a fonte REMOVIDA nao volta do disco.
//
// Relato do dono (Discord): removeu a conta Xtream e os addons e o guia
// continuava mostrando "714 canais" — o cache por fonte em disco nao sabia
// que a fonte tinha saido da configuracao. A regra agora:
//
//   cacheLer() pula a chave que a configuracao atual nao justifica;
//   a fonte que so FALHOU a rede continua configurada e entra normal;
//   guia_fonte_esquecer() apaga o arquivo e some os canais na hora;
//   guia_limpar_lista() clears all profile list caches and reloads sources.
//
// Sem rede e com disco de verdade: dados.c le e grava num diretorio de teste.
#define NV_PERFIL_SESSAO_FORTE
#include <unistd.h>
#ifndef GUIACACHE_SOURCE
#define GUIACACHE_SOURCE "../src/guia.c"
#endif
#include GUIACACHE_SOURCE
#include <assert.h>

// --- dubles: a "configuracao atual" mora aqui --------------------------------
static int nAddons;
static const char *bases[8];
static int xtOk, stOk;
int perfis_ativo(void) { return 1; }
int addons_n(void) { return nAddons; }
const char *addons_base(int i) { return bases[i]; }
const char *addons_nome(int i) { (void)i; return "addon"; }
int addons_ativo(int i) { (void)i; return 1; }
int addons_base_desligada(const char *b) { (void)b; return 0; }
int addons_sondado(int i) { (void)i; return 1; }
int addons_catalogos_canal(int i, AddCatCanal *s, int m) { (void)i; (void)s; (void)m; return 0; }
int addons_ocupado(void) { return 1; }   // 1 = prefetch nao atrapalha
int player_carregando(void) { return 0; }
int addons_consultar(const char *id, const char *tipo, const char *base, int fios,
                     int (*cancelado)(void *), void *ctx, Stream **saida) {
  (void)id; (void)tipo; (void)base; (void)fios; (void)cancelado; (void)ctx;
  *saida = NULL; return 0;
}
int addons_consultar_sessao(const char *id, const char *tipo, const char *base, int fios,
                            int (*cancelado)(void *), void *ctx, Stream **saida, uint64_t se) {
  (void)se;
  return addons_consultar(id, tipo, base, fios, cancelado, ctx, saida);
}
const char *ajustes_epg_pais(void) { return ""; }
int ajustes_idioma(void) { return 0; }
const char *ajustes_tmdb_idioma(void) { return "pt-BR"; }
int xtream_e_id(const char *id) { (void)id; return 0; }
Uint32 SDL_GetTicks(void) { static Uint32 t = 1000; return t += 16; }
int xtream_configurado(void) { return xtOk; }
int stalker_configurado(void) { return stOk; }
int xtream_url_xmltv(char *d, unsigned n) { (void)d; (void)n; return 0; }
int xtream_conta(XtreamConta *c) { (void)c; return 0; }
uint64_t perfis_sessao_geracao(void) { return 0; }
int perfis_sessao_atual(uint64_t g) { return g == 0; }
int perfis_sessao_travar(uint64_t g) { (void)g; return 1; }
void perfis_sessao_destravar(void) {}
const char *i18n(const char *s) { return s; }

// Rede: nunca responde (o que vale aqui e o cache, nao a carga).
char *rede_baixar(const char *u, int s) { (void)u; (void)s; return NULL; }
char *rede_baixar_medido_controle(const char *u, int s, const char *const *c,
                                  const RedeControle *ct, RedeMedida *m) {
  (void)u; (void)s; (void)c; (void)ct; (void)m; return NULL;
}
int rede_baixar_fluxo(const char *u, int s, const char *const *c,
                      int (*cb)(const char *, size_t, void *), void *x,
                      long long max, int *st, long long *bytes) {
  (void)u; (void)s; (void)c; (void)cb; (void)x; (void)max;
  if (st) *st = 0; if (bytes) *bytes = 0;
  return 0;
}
int stalker_canais_sessao(StalkerCanal *s, int m, uint64_t se) { (void)s; (void)m; (void)se; return 0; }
int xtream_canais_cb_sessao(void (*cat)(const char *, int, int, void *),
                            XtreamCb cb, void *u, int *total, uint64_t sessao) {
  (void)cat; (void)cb; (void)u; (void)sessao;
  if (total) *total = 0;
  return 0;
}

// Tex/orcamento so para ltv_teto_canais nao consultar nada.
void tex_orcamento_info(int *mb, long *memTotal, int *fixo, int *slots) {
  if (mb) *mb = 0; if (memTotal) *memTotal = 4096;
  if (fixo) *fixo = 0; if (slots) *slots = 0;
}
PtvPlataforma ptv_plataforma(void) { return PTV_LG; }

// --- semeia o cache no disco pelo caminho de verdade -------------------------
static void semearFonteCache(const char *base, const char *grupo, const char **nomes, int n) {
  int i, fl;
  stagingZerar();
  sTeto = 30000;
  fl = sFonte(LTV_ADDON, base, base, base);
  assert(fl >= 0);
  for (i = 0; i < n; i++) {
    char id[32];
    GCru c;
    memset(&c, 0, sizeof c);
    snprintf(id, sizeof id, "%s-%d", base, i);
    c.id = id; c.nome = nomes[i]; c.logo = c.desc = "";
    c.epgId = c.tvgNome = c.url = c.ua = c.ref = "";
    c.grupo = grupo; c.numero = i + 1;
    sGrupoInfo(fl, grupo, 1);
    assert(sCanalAdd(fl, &c));
    sLtvF[fl].total++;
  }
  adotarStaging();          // staging -> publicado (canais/ltvF)
  cacheGravar();            // grava ltv-<chave>-p1.bin + indice
}

static int temCanal(const char *nome) {
  int i;
  for (i = 0; i < nCanais; i++) if (!strcmp(canais[i].nome, nome)) return 1;
  return 0;
}

static void resetPublished(void) {
  fontesLtvSoltar(ltvF, nLtvF); nLtvF = 0;
  free(canais); canais = NULL; nCanais = capCanais = 0;
  free(cats); cats = NULL; nCats = capCats = 0;
  ltv_arena_soltar(&arena);
}

int main(int argc, char **argv) {
  static const char *a[] = { "Canal A1", "Canal A2" }, *b[] = { "Canal B1" };
  char chA[LTV_CHAVE], chB[LTV_CHAVE], nome[64], caminho[600];
  assert(argc == 2);
  dados_iniciar(argv[1]);
  bases[0] = "https://a.example"; bases[1] = "https://b.example";
  nAddons = 2;
  ltv_chave_fonte(LTV_ADDON, bases[0], chA, sizeof chA);
  ltv_chave_fonte(LTV_ADDON, bases[1], chB, sizeof chB);
  semearFonteCache(bases[0], "Grupo", a, 2);
  semearFonteCache(bases[1], "Grupo", b, 1);
  // Each seed publishes one source; index must retain both fixture sources.
  cacheNomeIndice(nome, sizeof nome);
  { char idx[128]; snprintf(idx, sizeof idx, "%s\n%s\n", chA, chB); dados_gravar(nome, idx); }
  resetPublished();
  assert(cacheLer() && nCanais == 3 && nLtvF == 2);
  assert(temCanal("Canal A1") && temCanal("Canal B1"));
#ifndef GUIACACHE_BASELINE
  // Remove first source in memory: B's channel and category indices must move.
  guia_fonte_esquecer(chA);
  assert(nCanais == 1 && nLtvF == 1 && canais[0].fonte == 0 && cats[0].fonte == 0);
  assert(!strcmp(chaveDo(&canais[0]), chB));
  semearFonteCache(bases[0], "Grupo", a, 2);
  cacheNomeIndice(nome, sizeof nome);
  { char idx[128]; snprintf(idx, sizeof idx, "%s\n%s\n", chA, chB); dados_gravar(nome, idx); }
#endif
  resetPublished();
  // Only B remains configured. A must never return from disk.
  bases[0] = bases[1]; nAddons = 1;
  assert(cacheLer());
  assert(nCanais == 1 && nLtvF == 1);
  assert(!temCanal("Canal A1") && temCanal("Canal B1"));
  // Real failed-network recovery path preserves configured B's disk list.
  stagingZerar(); sTeto = 30000;
  assert(rede_baixar(bases[0], 1) == NULL);
  fonteVaziaRecupera(LTV_ADDON, bases[0], "B", bases[0]);
  assert(sNCanais == 1 && sLtvF[0].deCache && sLtvF[0].saude == LTV_SAUDE_DEGRADADA);
  stagingZerar();
#ifndef GUIACACHE_BASELINE
  // Pending network results must be cancelled; reset also wipes B's cache.
  fioVivo = 1;
  guia_limpar_lista();
  assert(cargaCancelada && recarregarPend && !nCanais && !nLtvF);
  cacheNomeFonte(chB, nome, sizeof nome);
  assert(dados_caminho(caminho, sizeof caminho, nome));
  assert(access(caminho, F_OK) != 0);
  fioVivo = 0;
#endif
  puts("PASS: removed A absent; configured B survives network failure; explicit reset clears cache");
  return 0;
}
