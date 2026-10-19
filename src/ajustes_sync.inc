// Portable C preferences have their own versioned section. Web keys keep
// their original types; unknown web features survive the textual merge.
static int ajustePortavel(int op) {
  if (OPCOES[op].tipo != OP_ESCOLHA && OPCOES[op].tipo != OP_NUMERO) return 0;
  if (ajLegendaPrincipal(op)) return 1;
  if (!CHAVE[op] || CHAVE[op][0] == '-') return 0;
  switch (op) {
    case AJ_AMPLIAR_UI: case AJ_RESOLUCAO: case AJ_TEX_MB: case AJ_QUALIDADE_IMG:
    case AJ_GPU_EFEITOS: case AJ_ITENS_FILEIRA:
    case AJ_TAMANHO_UI: case AJ_TAMANHO_AJUSTES: case AJ_ESCALA_TEXTO:
    case AJ_INTERFACE_MODO: case AJ_MIC_BUSCA: case AJ_TECLADO_ANDROID:
    case AJ_HDR_UI: case AJ_ESMAECER: case AJ_BRILHO_PLAYER:
    case AJ_PROPORCAO_PADRAO: case AJ_LOGO_TRAILER:
    case AJ_DV: case AJ_ATMOS:
    case AJ_DTS_AC3: case AJ_DV_MKV: case AJ_TRAILER_ZOOM_TPK:
    case AJ_CACHE_SEEK: case AJ_BUFFER: case AJ_LEG_SYNC_AUDIO:
    case AJ_RETOMADA_MORNA: case AJ_MANTER_PRAZO: case AJ_MANTER_VIDEO:
    case AJ_PLAYER_EXTERNO: case AJ_PLAYER_ANIME:
    case AJ_P2P_LIGADO: case AJ_P2P_LIMITE: case AJ_P2P_DESTINO:
    case AJ_JF_LIGADO: case AJ_POSTER_PROV:
    case AJ_LIVETV_RES: case AJ_LIVETV_FORMATO: case AJ_LIVETV_ESPERA:
    case AJ_LIVETV_MODO: case AJ_LIVETV_PROXY: case AJ_LIVETV_SUGERIR:
    case AJ_ENVIO_AUTO: case AJ_MEDIDOR: case AJ_ESTAT_TAMANHO: case AJ_ESTAT_LADO:
    case AJ_VIDRO_QUALIDADE: case AJ_ICONE_APP: case AJ_LOGO_APP: case AJ_ABERTURA:
      return 0;
    default: return 1;
  }
}

#include "ajustes_device.inc"

static void ajustesArquivoSync(int k, char *nome, size_t tam) {
  if (k == 0) snprintf(nome, tam, "fonteregra.txt");
  else if (k == 1) snprintf(nome, tam, "fileirasui-p%d.txt", perfis_ativo());
  else ajustes_provedores_arquivo(nome, (unsigned)tam, "fontefalha");
}

static char *ajustesExtras(void) {
  Jsw w;
  jsw_iniciar(&w); jsw_obj_ini(&w);
  jsw_chave(&w, "values"); jsw_obj_ini(&w);
  for (int i = 0; i < AJ_N; i++) if (ajustePortavel(i) && !ajusteAparelho(i)) {
    char key[96]; snprintf(key, sizeof key, "c_%s", CHAVE[i]);
    jsw_ci(&w, key, ajLegendaPrincipal(i) ? ajLegendaValor(i) : valor[i]);
  }
  jsw_obj_fim(&w);
  // These stores contain preferences only, never credentials. Filter legacy
  // row IDs derived from addon URLs: those can contain embedded tokens.
  for (int k = 0; k < 3; k++) {
    char nome[128] = "", key[24];
    ajustesArquivoSync(k, nome, sizeof nome);
    char *t = nome[0] ? dados_ler(nome) : NULL;
    snprintf(key, sizeof key, "store%d", k);
    if (k == 1 && t) {
      ajusteFileirasCampos(t, 0);
      char *out = t;
      for (char *p = t; *p;) {
        char *end = strchr(p, '\n');
        size_t n = end ? (size_t)(end + 1 - p) : strlen(p);
        const char *id = !strncmp(p, "linha ", 6) ? p + 6 :
                         !strncmp(p, "hero ", 5) ? p + 5 :
                         !strncmp(p, "visibilidade ", 13) ? p + 13 :
                         !strncmp(p, "addvence ", 9) ? p + 9 :
                         !strncmp(p, "nova ", 5) ? p + 5 :
                         !strncmp(p, "ordemfileira ", 13) ? p + 13 : NULL;
        int safe = 1;
        if (id) for (const char *q = id; q < p + n && *q != '\t' && *q != ' ' && *q != '\n'; q++)
          if (strchr(":/@?%=&", *q)) safe = 0;
        if (safe) { memmove(out, p, n); out += n; }
        p += n;
      }
      *out = 0;
    }
    // Hex keeps newlines/tabs exact with the shared display-text JSON reader.
    size_t len = t ? strlen(t) : 0;
    char *hex = malloc(len * 2 + 1);
    if (!hex) { free(t); jsw_livre(&w); return NULL; }
    for (size_t j = 0; j < len; j++) sprintf(hex + 2 * j, "%02x", (unsigned char)t[j]);
    hex[len * 2] = 0;
    jsw_cs(&w, key, hex); free(hex); free(t);
  }
  jsw_obj_fim(&w);
  const char *json = jsw_texto_final(&w);
  char *out = json ? strdup(json) : NULL;
  jsw_livre(&w); return out;
}

static int ajustesExtrasAplicar(const char *json) {
  const char *a, *b, *v, *f;
  // Resolve device fields and provider scope before choosing shared filenames.
  int mudou = ajustesDeviceAplicar(json);
  if (!acharValor(json, json + strlen(json), "c_settings_v1", &a, &b) || *a != '{' ||
      !acharValor(a, b, "values", &v, &f) || *v != '{') return mudou;
  ajustesSnapshotAtual();
  for (int i = 0; i < AJ_N; i++) if (ajustePortavel(i) && !ajusteAparelho(i)) {
    const char *p, *end;
    char key[96];
    snprintf(key, sizeof key, "c_%s", CHAVE[i]);
    if (!acharValor(v, f, key, &p, &end) || p == end) continue;
    char *tail; long num = strtol(p, &tail, 10);
    if (tail != end || num < INT_MIN || num > INT_MAX) continue;
    // Web edits win overlapping choices; C-only accents have no web value.
    if (!somenteDesteAparelho(i) &&
        !(i == AJ_TEMA && num >= AJ_N_TEMAS && num < AJ_N_TEMAS_OPC)) {
      camelParaSnake(chaveConta(i), key, sizeof key);
      if (acharValor(json, json + strlen(json), key, &p, &end) ||
          acharValor(json, json + strlen(json), chaveConta(i), &p, &end)) continue;
    }
    int novo = limita(i, (int)num);
    if (ajLegendaPrincipal(i)) { if (novo != ajLegendaValor(i)) mudou += ajLegendaDefinir(i, novo); }
    else if (valor[i] != novo) { valor[i] = novo; mudou++; }
  }
  for (int k = 0; k < 3; k++) {
    char key[24], nome[128] = "";
    snprintf(key, sizeof key, "store%d", k);
    if (!acharValor(a, b, key, &v, &f) || *v != '"') continue;
    // An escaped string never decodes to more bytes than its JSON input.
    char *t = malloc((size_t)(f - v) + 32);
    if (!t) continue;
    if (f - v < 2 || f[-1] != '"') { free(t); continue; }
    size_t len = (size_t)(f - v) - 2;
    int valid = len % 2 == 0;
    for (size_t j = 0; valid && j < len; j++)
      if (!strchr("0123456789abcdef", v[1 + j])) valid = 0;
    if (valid) {
      for (size_t j = 0; j < len / 2; j++) {
        unsigned byte; sscanf(v + 1 + j * 2, "%2x", &byte); t[j] = (char)byte;
        if (!byte) valid = 0;
      }
      t[len / 2] = 0;
    }
    if (valid) {
      ajustesArquivoSync(k, nome, sizeof nome);
      // Overlay only device fields, never replace newer shared rows.
      if (k == 1) {
        char *device = dados_ler(nome), *merged = ajusteFileirasMesclar(t, device);
        free(device); free(t); t = merged;
        if (!t) continue;
      }
      if (nome[0] && dados_gravar(nome, t)) {
        if (k == 0) fonteregra_carregar();
        else if (k == 1) fil_recarregar();
        else fontefalha_config_ler();
        mudou++;
      }
    }
    free(t);
  }
  aplicarIdioma(AJ_LEG_LINGUA); aplicarIdioma(AJ_LEG_LINGUA2);
  aplicarIdioma(AJ_AUD_LINGUA); aplicarIdioma(AJ_AUD_LINGUA2);
  if (valor[AJ_IDIOMA] == 0) idiomaEscolhido();
  txt_definir_fonte_interface((TxtFamilia)valor[AJ_FONTE_UI]);
  if (mudou) gravar();
  return mudou;
}

int ajustes_mesclar_blob(const char *base, char **saida) {
  char *web = NULL, *ext;
  if (saida) *saida = NULL;
  if (!base || !*base || !saida || *base != '{') return 0;
  int n = ajustesMesclarWeb(base, &web);
  const char *src = web ? web : base, *fim = js_fim(src);
  if (!fim || fim <= src + 1 || fim[-1] != '}' || !(ext = ajustesExtras())) { free(web); return 0; }
  Jsw patch; jsw_iniciar(&patch); jsw_obj_ini(&patch);
  jsw_chave(&patch, "c_settings_v1"); jsw_bruto(&patch, ext); free(ext);
  const char *id = dados_aparelho_id();
  if (*id) {
    char *device = ajustesDeviceSnapshot();
    if (!device) { jsw_livre(&patch); free(web); return 0; }
    jsw_chave(&patch, "c_devices_v1"); jsw_obj_ini(&patch);
    jsw_chave(&patch, id); jsw_bruto(&patch, device); free(device);
    jsw_obj_fim(&patch);
  }
  jsw_obj_fim(&patch);
  const char *p = jsw_texto_final(&patch);
  char *merged = p ? ajJsonMerge(src, fim, p, p + strlen(p)) : NULL;
  jsw_livre(&patch);
  if (!merged) { free(web); return 0; }
  if (!strcmp(merged, src)) { free(merged); *saida = web; return n; }
  *saida = merged; free(web);
  return n + 1;
}
