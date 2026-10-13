#include "fonteregra.h"
#include "dados.h"
#include "js.h"
#include "regexjs.h"
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// ESTADO: o que esta em uso nesta TV (fonteregra.txt). A copia por perfil
// (fonteregra-p<N>.txt) segue ajustes_perfil_guardar/_restaurar.
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static char padrao[FR_REGEX_MAX + 1];
static char nomes[2][FR_NOMES_MAX][FR_NOME_MAX];
static int nNomes[2];
// A ORDEM (add-ons e plugins numa fila so, pelo nome de exibicao).
static char ordem[FR_ORDEM_MAX][FR_NOME_MAX];
static int nOrdem;
static unsigned versao = 1;
// A regex compilada do padrao em uso, no motor de JS proprio (regexjs.c), e a
// exclusao "do oficial" (reJsX, ver excluirJs). `reEstado` e o de
// fonteregra_regex_estado.
static RegexJs *reJs, *reJsX;
static int reEstado;

#define ARQ "fonteregra.txt"

typedef struct { char *p; size_t tam, n; int cheio; } Buf;
static void bput(Buf *b, const char *s) {
  size_t k = strlen(s);
  if (b->n + k >= b->tam) { b->cheio = 1; return; }
  memcpy(b->p + b->n, s, k); b->n += k; b->p[b->n] = 0;
}
static void bputc(Buf *b, char c) { char s[2] = { c, 0 }; bput(b, s); }

// O oficial so liga o filtro com uma letra ou digito ASCII (isRegexSelectionConfigured:
// /[a-z0-9]/i): "🇪🇸|🇲🇽" ou "[ñÑ]" sozinhos la nao filtram nada, e aqui tambem nao.
static int temLetraOuDigito(const char *s) {
  for (; *s; s++) if (isalnum((unsigned char)*s) && (unsigned char)*s < 0x80) return 1;
  return 0;
}

// A EXCLUSAO DO OFICIAL (NuvioTVSmart, streamAutoPlaySelector.js buildExcludeRegex):
// alem de rodar o padrao inteiro, o oficial acha cada "(?!" com um "(" depois
// (\(\?![^)]*?\(([^)]+)\)), junta o PRIMEIRO grupo de cada um, separa por "|" e
// compila \b(palavras)\b. Casou essa regex = fonte descartada, mesmo que o
// lookahead de verdade nao pegasse. Se ela nao compila (por exemplo, "(?:" vira a
// palavra "?:\bDV\b"), o oficial fica sem exclusao nenhuma — e este codigo tambem.
// Devolve NULL quando nao ha palavras ou ela nao compila.
static RegexJs *excluirJs(const char *java) {
  char pal[FR_REGEX_MAX + 8], re[FR_REGEX_MAX + 24];
  size_t n = 0;
  const char *p = java;
  pal[0] = 0;
  while ((p = strstr(p, "(?!")) != NULL) {
    const char *q = p + 3, *fim, *a, *b;
    while (*q && *q != ')' && *q != '(') q++;
    if (*q != '(') { p++; continue; }
    fim = strchr(q + 1, ')');
    if (!fim || fim == q + 1) { p++; continue; }
    // split("|") + trim + filter(Boolean), tudo no mesmo passo.
    for (a = q + 1; a <= fim; ) {
      b = a;
      while (b < fim && *b != '|') b++;
      { const char *i = a, *j = b;
        while (i < j && (unsigned char)*i <= ' ') i++;
        while (j > i && (unsigned char)j[-1] <= ' ') j--;
        if (j > i) {
          if (n + (size_t)(j - i) + 2 >= sizeof pal) return NULL;
          if (n) pal[n++] = '|';
          memcpy(pal + n, i, (size_t)(j - i)); n += (size_t)(j - i);
          pal[n] = 0;
        } }
      a = b + 1;
    }
    p = fim + 1;
  }
  if (!n) return NULL;
  snprintf(re, sizeof re, "\\b(%s)\\b", pal);
  return regexjs_compilar(re, REGEXJS_I, NULL, 0);
}

// Compila padrao + exclusao. 1 = ok; 0 = nao configurado; -1 = invalido.
// TODO padrao roda no motor de JS, com a semantica EXATA do oficial (Android e
// web: new RegExp(p, "i") + buildExcludeRegex): o ponto NAO atravessa quebra de
// linha, entao "^(?=.*1080p)" so ve a primeira linha do texto, como la.
//
// Antes, padrao sem "(?=" / lookbehind ia para uma traducao POSIX que errava em
// casos comuns: "(?!.*CAM)1080p" e "1080p(?!.*DTS)" perdiam o lookahead (sem
// grupo dentro nao sobrava exclusao: CAM e DTS passavam), e o "\b" virava um
// caractere consumido, entao "\b(Remux|BluRay)\b.*\b2160p\b" recusava
// "Remux 2160p" (o espaco servia a uma borda so). O motor proprio agora e
// conferido contra o V8 (tests/regexjs_v8.sh) e lembra estados, entao nao ha
// mais motivo para dois caminhos.
static int compilar(const char *java, RegexJs **js, RegexJs **jsx) {
  char p[FR_REGEX_MAX + 8];
  *js = *jsx = NULL;
  if (!java || !java[0] || !temLetraOuDigito(java)) return 0;
  // "(?i)" na frente (Java/Python) e aceito; o resto e sem caixa de qualquer jeito.
  snprintf(p, sizeof p, "(?i)%s", java);
  *js = regexjs_compilar_web(p, NULL, 0);
  if (!*js) return -1;
  *jsx = excluirJs(java);
  return 1;
}

static int casa(const RegexJs *js, const RegexJs *jsx, const char *texto) {
  size_t n;
  if (!js) return 0;
  if (!texto) texto = "";
  n = strlen(texto);
  if (!regexjs_testar(js, texto, n)) return 0;
  return !(jsx && regexjs_testar(jsx, texto, n));
}

int fonteregra_regex_testar(const char *java, const char *texto) {
  RegexJs *js, *jsx;
  int e = compilar(java, &js, &jsx), m;
  if (e <= 0) return -1;
  m = casa(js, jsx, texto);
  regexjs_liberar(js);
  regexjs_liberar(jsx);
  return m;
}

// --- modelos ----------------------------------------------------------------
// Padroes de idioma como os addons escrevem (AIOStreams, Torrentio, MediaFusion
// usam bandeira, sigla ou o nome). O de 4K e o exemplo do proprio oficial.
static const char *const MODELOS[] = {
  "",
  "\\bESP\\b|\\bSPA\\b|Spanish|Espa(ñ|n)ol|Castellano|Latino|LATAM|🇪🇸|🇲🇽",
  "PT-?BR|Portugu|Dublado|\\bDUB\\b|🇧🇷|🇵🇹",
  "\\bENG?\\b|English|🇬🇧|🇺🇸",
  "\\bMULTI\\b",
  "\\bDUAL\\b",
  "4K|2160p|Remux",
};
int fonteregra_modelos(void) { return (int)(sizeof MODELOS / sizeof *MODELOS); }
const char *fonteregra_modelo(int i) { return i > 0 && i < fonteregra_modelos() ? MODELOS[i] : ""; }

// --- estado -----------------------------------------------------------------
static void recompilarSemTrava(void) {
  regexjs_liberar(reJs); reJs = NULL;
  regexjs_liberar(reJsX); reJsX = NULL;
  reEstado = compilar(padrao, &reJs, &reJsX);
}

static void limparNome(char *dst, size_t tam, const char *src) {
  size_t k = 0;
  while (src && *src && (unsigned char)*src <= ' ') src++;
  for (; src && *src && k + 1 < tam; src++)
    dst[k++] = (*src == '\n' || *src == '\r' || *src == '\t') ? ' ' : *src;
  while (k && (unsigned char)dst[k - 1] <= ' ') k--;
  dst[k] = 0;
}

static int achaSemTrava(int plugin, const char *nome) {
  int k;
  for (k = 0; k < nNomes[plugin]; k++) if (!strcasecmp(nomes[plugin][k], nome)) return k;
  return -1;
}
static void poeSemTrava(int plugin, const char *nome) {
  char n[FR_NOME_MAX];
  limparNome(n, sizeof n, nome);
  if (!n[0] || achaSemTrava(plugin, n) >= 0 || nNomes[plugin] >= FR_NOMES_MAX) return;
  snprintf(nomes[plugin][nNomes[plugin]++], FR_NOME_MAX, "%s", n);
}

static int achaOrdemSemTrava(const char *nome) {
  int k;
  for (k = 0; k < nOrdem; k++) if (!strcasecmp(ordem[k], nome)) return k;
  return -1;
}
static void poeOrdemSemTrava(const char *nome) {
  char n[FR_NOME_MAX];
  limparNome(n, sizeof n, nome);
  if (!n[0] || achaOrdemSemTrava(n) >= 0 || nOrdem >= FR_ORDEM_MAX) return;
  snprintf(ordem[nOrdem++], FR_NOME_MAX, "%s", n);
}

static char *serializarSemTrava(void) {
  size_t cap = 64 + strlen(padrao) + (size_t)(nNomes[0] + nNomes[1] + nOrdem) * (FR_NOME_MAX + 8), w = 0;
  char *t = malloc(cap);
  int p, k;
  if (!t) return NULL;
  w += (size_t)snprintf(t + w, cap - w, "regex %s\n", padrao);
  for (p = 0; p < 2; p++)
    for (k = 0; k < nNomes[p]; k++)
      w += (size_t)snprintf(t + w, cap - w, "%s %s\n", p ? "plugin" : "addon", nomes[p][k]);
  for (k = 0; k < nOrdem; k++) w += (size_t)snprintf(t + w, cap - w, "ordem %s\n", ordem[k]);
  return t;
}

static void lerSemTrava(const char *t) {
  const char *l = t;
  padrao[0] = 0; nNomes[0] = nNomes[1] = 0; nOrdem = 0;
  while (l && *l) {
    const char *f = strchr(l, '\n');
    size_t n = f ? (size_t)(f - l) : strlen(l);
    char linha[FR_REGEX_MAX + 16];
    if (n >= sizeof linha) n = sizeof linha - 1;
    memcpy(linha, l, n); linha[n] = 0;
    if (n && linha[n - 1] == '\r') linha[n - 1] = 0;
    if (!strncmp(linha, "regex ", 6)) snprintf(padrao, sizeof padrao, "%s", linha + 6);
    else if (!strncmp(linha, "addon ", 6)) poeSemTrava(0, linha + 6);
    else if (!strncmp(linha, "plugin ", 7)) poeSemTrava(1, linha + 7);
    else if (!strncmp(linha, "ordem ", 6)) poeOrdemSemTrava(linha + 6);
    l = f ? f + 1 : NULL;
  }
  recompilarSemTrava();
  versao++;
}

static void gravarSemTrava(const char *nome) {
  char *t = serializarSemTrava();
  if (t) { dados_gravar(nome, t); free(t); }
}

void fonteregra_carregar(void) {
  char *t = dados_ler(ARQ);
  pthread_mutex_lock(&trava);
  lerSemTrava(t ? t : "");
  pthread_mutex_unlock(&trava);
  free(t);
}

unsigned fonteregra_versao(void) {
  unsigned v;
  pthread_mutex_lock(&trava); v = versao; pthread_mutex_unlock(&trava);
  return v;
}

void fonteregra_regex(char *dst, size_t tam) {
  if (!dst || !tam) return;
  pthread_mutex_lock(&trava);
  snprintf(dst, tam, "%s", padrao);
  pthread_mutex_unlock(&trava);
}

int fonteregra_definir_regex(const char *p) {
  char novo[FR_REGEX_MAX + 1];
  int e;
  limparNome(novo, sizeof novo, p ? p : "");
  pthread_mutex_lock(&trava);
  if (strcmp(novo, padrao)) {
    snprintf(padrao, sizeof padrao, "%s", novo);
    recompilarSemTrava();
    versao++;
    gravarSemTrava(ARQ);
  }
  e = reEstado;
  pthread_mutex_unlock(&trava);
  return e;
}

int fonteregra_regex_estado(void) {
  int e;
  pthread_mutex_lock(&trava); e = reEstado; pthread_mutex_unlock(&trava);
  return e;
}

int fonteregra_n(int plugin) {
  int n;
  plugin = plugin ? 1 : 0;
  pthread_mutex_lock(&trava); n = nNomes[plugin]; pthread_mutex_unlock(&trava);
  return n;
}

int fonteregra_nome(int plugin, int k, char *dst, size_t tam) {
  int ok = 0;
  plugin = plugin ? 1 : 0;
  if (!dst || !tam) return 0;
  dst[0] = 0;
  pthread_mutex_lock(&trava);
  if (k >= 0 && k < nNomes[plugin]) { snprintf(dst, tam, "%s", nomes[plugin][k]); ok = 1; }
  pthread_mutex_unlock(&trava);
  return ok;
}

int fonteregra_contem(int plugin, const char *nome) {
  int r;
  plugin = plugin ? 1 : 0;
  if (!nome) return 0;
  pthread_mutex_lock(&trava); r = achaSemTrava(plugin, nome) >= 0; pthread_mutex_unlock(&trava);
  return r;
}

int fonteregra_alternar(int plugin, const char *nomeCru) {
  char nome[FR_NOME_MAX];
  int k, r;
  plugin = plugin ? 1 : 0;
  limparNome(nome, sizeof nome, nomeCru);
  if (!nome[0]) return 0;
  pthread_mutex_lock(&trava);
  k = achaSemTrava(plugin, nome);
  if (k >= 0) {
    memmove(nomes[plugin][k], nomes[plugin][k + 1], (size_t)(nNomes[plugin] - k - 1) * FR_NOME_MAX);
    nNomes[plugin]--;
    r = 0;
  } else {
    poeSemTrava(plugin, nome);
    r = achaSemTrava(plugin, nome) >= 0;
  }
  versao++;
  gravarSemTrava(ARQ);
  pthread_mutex_unlock(&trava);
  return r;
}

void fonteregra_limpar(int plugin) {
  plugin = plugin ? 1 : 0;
  pthread_mutex_lock(&trava);
  if (nNomes[plugin]) { nNomes[plugin] = 0; versao++; gravarSemTrava(ARQ); }
  pthread_mutex_unlock(&trava);
}

int fonteregra_modelo_atual(void) {
  int i, r = 0;
  pthread_mutex_lock(&trava);
  for (i = 1; i < fonteregra_modelos() && padrao[0]; i++) if (!strcmp(padrao, MODELOS[i])) { r = i; break; }
  pthread_mutex_unlock(&trava);
  return r;
}

// --- a ordem dos add-ons (so local, o oficial nao tem) ---------------------
int fonteregra_ordem_n(void) {
  int n;
  pthread_mutex_lock(&trava); n = nOrdem; pthread_mutex_unlock(&trava);
  return n;
}
int fonteregra_ordem_nome(int k, char *dst, size_t tam) {
  int ok = 0;
  if (!dst || !tam) return 0;
  dst[0] = 0;
  pthread_mutex_lock(&trava);
  if (k >= 0 && k < nOrdem) { snprintf(dst, tam, "%s", ordem[k]); ok = 1; }
  pthread_mutex_unlock(&trava);
  return ok;
}
void fonteregra_ordem_definir(const char *const *nomesNovos, int n) {
  int k;
  pthread_mutex_lock(&trava);
  nOrdem = 0;
  for (k = 0; k < n && nomesNovos; k++) poeOrdemSemTrava(nomesNovos[k]);
  versao++;
  gravarSemTrava(ARQ);
  pthread_mutex_unlock(&trava);
}
int fonteregra_ordem_rank(const char *nome) {
  int k;
  if (!nome || !nome[0]) return FR_ORDEM_SEM;
  pthread_mutex_lock(&trava); k = achaOrdemSemTrava(nome); pthread_mutex_unlock(&trava);
  return k < 0 ? FR_ORDEM_SEM : k;
}
void fonteregra_ordem_texto(char *dst, size_t tam) {
  int k;
  size_t w = 0;
  if (!dst || !tam) return;
  dst[0] = 0;
  pthread_mutex_lock(&trava);
  for (k = 0; k < nOrdem && w + 1 < tam; k++)
    w += (size_t)snprintf(dst + w, tam - w, "%s%s", k ? " \xE2\x80\xBA " : "", ordem[k]);
  pthread_mutex_unlock(&trava);
}

// --- a regra ----------------------------------------------------------------
// O ESCOPO E FILTRO, NAO PREFERENCIA (bloqueador 2.0.3, TCL do dono 21:24).
// "Somente add-ons instalados" com "usar os outros" ligado deixava o plugin no
// grupo 2 e, sem fonte de add-on na lista, "MegaEmbed - 1080" tocou ("winner
// other+match"). "Os outros" e a folga das listas de PERMITIDOS (o add-on que a
// pessoa nao marcou); fonte fora do escopo nunca entra no automatico — como o
// "Auto-play Source Scope" do oficial. Sem fonte no escopo, a espera segue ou
// a lista de fontes abre (regraBloqueou em streams.c).
static int foraDoEscopo(const FonteRegraCfg *c, int plugin) {
  return (c->escopo == FR_ESCOPO_ADDONS && plugin) ||
         (c->escopo == FR_ESCOPO_PLUGINS && !plugin);
}
int fonteregra_no_escopo(const FonteRegraCfg *c, int plugin) {
  return !c || !foraDoEscopo(c, plugin ? 1 : 0);
}
static int permitidoSemTrava(const FonteRegraCfg *c, const char *nome, int plugin) {
  plugin = plugin ? 1 : 0;
  if (foraDoEscopo(c, plugin)) return 0;
  return !nNomes[plugin] || achaSemTrava(plugin, nome ? nome : "") >= 0;
}

int fonteregra_grupo(const FonteRegraCfg *c, const char *nome, int plugin, const char *texto) {
  int perm, sub = 0;
  if (!c) return 0;
  if (foraDoEscopo(c, plugin ? 1 : 0)) return -1;
  pthread_mutex_lock(&trava);
  perm = permitidoSemTrava(c, nome, plugin);
  if (c->regexModo != FR_REGEX_DESLIGADA && reEstado == 1) {
    int m = casa(reJs, reJsX, texto);
    if (!m && c->regexModo == FR_REGEX_EXIGIR) { pthread_mutex_unlock(&trava); return -1; }
    sub = !m;
  }
  pthread_mutex_unlock(&trava);
  if (!perm && !c->usarOutros) return -1;
  return (perm ? 0 : 2) + sub;
}

int fonteregra_grupo_pendente(const FonteRegraCfg *c, const char *nome, int plugin) {
  int perm;
  if (!c) return 0;
  if (foraDoEscopo(c, plugin ? 1 : 0)) return -1;
  pthread_mutex_lock(&trava);
  perm = permitidoSemTrava(c, nome, plugin);
  pthread_mutex_unlock(&trava);
  return perm ? 0 : c->usarOutros ? 2 : -1;
}

int fonteregra_ativa(const FonteRegraCfg *c) {
  int r;
  if (!c) return 0;
  pthread_mutex_lock(&trava);
  r = c->escopo != FR_ESCOPO_TODAS || nNomes[0] || nNomes[1] ||
      (c->regexModo != FR_REGEX_DESLIGADA && reEstado == 1);
  pthread_mutex_unlock(&trava);
  return r;
}

// --- perfil -----------------------------------------------------------------
static void nomePerfil(char *dst, size_t tam, int perfil) { snprintf(dst, tam, "fonteregra-p%d.txt", perfil); }

void fonteregra_perfil_guardar(int perfil) {
  char nome[40];
  if (perfil <= 0) return;
  nomePerfil(nome, sizeof nome, perfil);
  pthread_mutex_lock(&trava); gravarSemTrava(nome); pthread_mutex_unlock(&trava);
}

int fonteregra_perfil_restaurar(int perfil) {
  char nome[40], *t;
  if (perfil <= 0) return 0;
  nomePerfil(nome, sizeof nome, perfil);
  if (!(t = dados_ler(nome))) return 0;
  pthread_mutex_lock(&trava);
  lerSemTrava(t);
  gravarSemTrava(ARQ);
  pthread_mutex_unlock(&trava);
  free(t);
  return 1;
}

void fonteregra_perfil_esquecer(void) {
  char nome[40];
  int i;
  for (i = 1; i <= 32; i++) { nomePerfil(nome, sizeof nome, i); dados_apagar(nome); }
}

// --- conta ------------------------------------------------------------------
static const char *const K_REGEX = "stream_auto_play_regex";
static const char *const K_LISTA[2] = { "stream_auto_play_selected_addons", "stream_auto_play_selected_plugins" };

// O valor de `chave`, desembrulhado de {"type":..,"value":X}, em `dst`.
static int brutoDesembrulhado(const char *json, const char *chave, char *dst, size_t tam) {
  const char *fim = json + strlen(json);
  if (!js_bruto(json, fim, chave, dst, tam)) return 0;
  if (dst[0] == '{') {
    char *v = malloc(tam);
    int ok = v && js_bruto(dst, dst + strlen(dst), "value", v, tam);
    if (ok) snprintf(dst, tam, "%s", v);
    free(v);
    return ok;
  }
  return 1;
}

int fonteregra_do_blob(const char *json) {
  size_t tam = 32768;
  char *b, txt[FR_REGEX_MAX + 1];
  int mudou = 0, p;
  if (!json || !*json || !(b = malloc(tam))) return 0;
  pthread_mutex_lock(&trava);
  if (brutoDesembrulhado(json, K_REGEX, b, tam) && b[0] == '"' && js_cadeia(b, txt, sizeof txt)) {
    char limpo[FR_REGEX_MAX + 1];
    limparNome(limpo, sizeof limpo, txt);
    if (strcmp(limpo, padrao)) { snprintf(padrao, sizeof padrao, "%s", limpo); recompilarSemTrava(); mudou++; }
  }
  for (p = 0; p < 2; p++) {
    char antes[FR_NOMES_MAX][FR_NOME_MAX];
    int nAntes = nNomes[p], k, igual;
    const char *e;
    if (!brutoDesembrulhado(json, K_LISTA[p], b, tam) || b[0] != '[') continue;
    memcpy(antes, nomes[p], sizeof antes);
    nNomes[p] = 0;
    for (e = js_raiz_array(b); e; ) {
      char nm[FR_NOME_MAX];
      const char *q;
      if (*e != '"') { e = js_fim(e); e = e ? js_prox(e) : NULL; continue; }   // objeto: pula
      if (js_cadeia(e, nm, sizeof nm)) poeSemTrava(p, nm);
      for (q = e + 1; *q && *q != '"'; q += (*q == '\\' && q[1]) ? 2 : 1) {}
      e = *q ? js_prox(q + 1) : NULL;
    }
    igual = nAntes == nNomes[p];
    for (k = 0; igual && k < nAntes; k++) if (strcmp(antes[k], nomes[p][k])) igual = 0;
    if (!igual) mudou++;
  }
  if (mudou) { versao++; gravarSemTrava(ARQ); }
  pthread_mutex_unlock(&trava);
  free(b);
  if (mudou) { printf("[fonteregra] conta: %d campo(s) do auto-play mudaram\n", mudou); fflush(stdout); }
  return mudou;
}

// Onde esta o valor de `chave` em [ini,fim): *vi no primeiro caractere, *vf
// depois do ultimo. Mesma regra de acharValor em ajustes.c.
static int acharValor(const char *ini, const char *fim, const char *chave, const char **vi, const char **vf) {
  char alvo[96];
  const char *p;
  size_t n = (size_t)snprintf(alvo, sizeof alvo, "\"%s\"", chave);
  for (p = ini; (p = strstr(p, alvo)) != NULL && p < fim; p += n) {
    const char *v = p + n;
    while (v < fim && (unsigned char)*v <= ' ') v++;
    if (v >= fim || *v != ':') continue;
    v++;
    while (v < fim && (unsigned char)*v <= ' ') v++;
    if (v >= fim) return 0;
    if (*v == '{' || *v == '[') *vf = js_fim(v);
    else if (*v == '"') {
      const char *q = v + 1;
      while (q < fim && *q != '"') q += (*q == '\\' && q + 1 < fim) ? 2 : 1;
      *vf = q < fim ? q + 1 : fim;
    } else {
      const char *q = v;
      while (q < fim && *q != ',' && *q != '}' && *q != ']' && (unsigned char)*q > ' ') q++;
      *vf = q;
    }
    if (!*vf || *vf > fim) return 0;
    *vi = v;
    return 1;
  }
  return 0;
}

static void jsonTexto(Buf *b, const char *s) {
  bputc(b, '"');
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if (c == '"' || c == '\\') { bputc(b, '\\'); bputc(b, (char)c); }
    else if (c < 0x20) { char e[8]; snprintf(e, sizeof e, "\\u%04x", c); bput(b, e); }
    else bputc(b, (char)c);
  }
  bputc(b, '"');
}

typedef struct { const char *vi, *vf; char *texto; } Troca;

int fonteregra_mesclar(const char *base, char **saida) {
  Troca t[3];
  int n = 0, i, j;
  const char *fim, *p;
  size_t cap, w = 0;
  char *out;
  if (saida) *saida = NULL;
  if (!base || !*base || !saida) return 0;
  fim = base + strlen(base);
  pthread_mutex_lock(&trava);
  for (i = 0; i < 3; i++) {
    const char *chave = i == 0 ? K_REGEX : K_LISTA[i - 1], *vi, *vf, *ei, *ef;
    size_t tam = 64 + strlen(padrao) * 6 + (size_t)FR_NOMES_MAX * (FR_NOME_MAX * 6 + 4);
    Buf b;
    if (!acharValor(base, fim, chave, &vi, &vf)) continue;
    if (*vi == '{' && acharValor(vi, vf, "value", &ei, &ef)) { vi = ei; vf = ef; }
    // MESMO TIPO DO QUE O SERVIDOR GUARDA: texto para a regex, array para as
    // listas. Qualquer outra forma fica como esta.
    if ((i == 0 && *vi != '"') || (i > 0 && *vi != '[')) continue;
    b.p = malloc(tam); b.tam = tam; b.n = 0; b.cheio = 0;
    if (!b.p) continue;
    b.p[0] = 0;
    if (i == 0) jsonTexto(&b, padrao);
    else {
      int k, pl = i - 1;
      bputc(&b, '[');
      for (k = 0; k < nNomes[pl]; k++) { if (k) bputc(&b, ','); jsonTexto(&b, nomes[pl][k]); }
      bputc(&b, ']');
    }
    if (b.cheio || ((size_t)(vf - vi) == b.n && !strncmp(vi, b.p, b.n))) {
      // Ja igual: nao entra. Para array, a forma do servidor pode ter
      // espacos; compara sem eles antes de desistir de "igual".
      free(b.p); continue;
    }
    if (i > 0) {
      // Igual a menos de espacos fora de texto (o servidor pode indentar).
      char *a = malloc((size_t)(vf - vi) + 1);
      size_t q, k2 = 0; int dentro = 0;
      if (a) {
        for (q = 0; q < (size_t)(vf - vi); q++) {
          char c = vi[q];
          if (c == '"' && (q == 0 || vi[q - 1] != '\\')) dentro = !dentro;
          if (!dentro && (unsigned char)c <= ' ') continue;
          a[k2++] = c;
        }
        a[k2] = 0;
        if (!strcmp(a, b.p)) { free(a); free(b.p); continue; }
        free(a);
      }
    }
    t[n].vi = vi; t[n].vf = vf; t[n].texto = b.p; n++;
  }
  pthread_mutex_unlock(&trava);
  if (!n) return 0;
  for (i = 1; i < n; i++)
    for (j = i; j > 0 && t[j - 1].vi > t[j].vi; j--) { Troca x = t[j - 1]; t[j - 1] = t[j]; t[j] = x; }
  cap = strlen(base) + 1;
  for (i = 0; i < n; i++) cap += strlen(t[i].texto);
  out = malloc(cap);
  if (out) {
    p = base;
    for (i = 0; i < n; i++) {
      size_t pre = (size_t)(t[i].vi - p), k = strlen(t[i].texto);
      memcpy(out + w, p, pre); w += pre;
      memcpy(out + w, t[i].texto, k); w += k;
      p = t[i].vf;
    }
    memcpy(out + w, p, (size_t)(fim - p)); w += (size_t)(fim - p);
    out[w] = 0;
  }
  for (i = 0; i < n; i++) free(t[i].texto);
  if (!out) return 0;
  *saida = out;
  return n;
}
