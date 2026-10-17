// Ver partes.h para o porque.
#include "partes.h"
#include "debrid.h"
#include "dados.h"
#include "js.h"
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

int partes_disponivel(void) {
#ifdef NV_TPK
  return 1;
#else
  return 0;
#endif
}

// ---------------------------------------------------------------- JSON
//
// O valor de `chave` NA RAIZ do objeto que comeca em `ob` ('{'). O item do
// mylist tem "id", "hash" e "name" na raiz e de novo dentro de cada arquivo:
// js_texto/js_num pegam o primeiro que aparece, que pode ser o do arquivo.
static const char *raiz(const char *ob, const char *fim, const char *chave) {
  size_t L = strlen(chave);
  int prof = 0;
  const char *p = ob;
  if (!p || *p != '{') return NULL;
  if (!fim) fim = p + strlen(p);
  for (; p < fim && *p; p++) {
    char c = *p;
    if (c == '"') {
      const char *ini = p + 1, *q = ini;
      while (q < fim && *q && *q != '"') { if (*q == '\\' && q + 1 < fim && q[1]) q++; q++; }
      if (q >= fim || !*q) return NULL;
      if (prof == 1 && (size_t)(q - ini) == L && !memcmp(ini, chave, L)) {
        const char *v = q + 1;
        while (v < fim && isspace((unsigned char)*v)) v++;
        if (v < fim && *v == ':') {
          v++;
          while (v < fim && isspace((unsigned char)*v)) v++;
          return v < fim ? v : NULL;
        }
      }
      p = q;
      continue;
    }
    if (c == '{' || c == '[') prof++;
    else if (c == '}' || c == ']') { if (--prof <= 0) return NULL; }
  }
  return NULL;
}
static int textoRaiz(const char *ob, const char *fim, const char *chave, char *dst, size_t n) {
  const char *v = raiz(ob, fim, chave);
  if (n) dst[0] = 0;
  return v && *v == '"' && js_cadeia(v, dst, n);
}
static double numRaiz(const char *ob, const char *fim, const char *chave, double padrao) {
  const char *v = raiz(ob, fim, chave);
  char *e;
  double d;
  if (!v || !(*v == '-' || isdigit((unsigned char)*v))) return padrao;
  d = strtod(v, &e);
  return e != v && isfinite(d) ? d : padrao;
}

// ---------------------------------------------------------------- puro

static int ehHex(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

int partes_hashes_texto(const char *s, char saida[][41], int max) {
  int k = 0;
  if (!s) return 0;
  for (const char *p = s; *p && k < max; ) {
    if (!ehHex((unsigned char)*p)) { p++; continue; }
    const char *q = p;
    while (*q && ehHex((unsigned char)*q)) q++;
    if (q - p == 40) {
      int dup = 0;
      for (int i = 0; i < 40; i++) saida[k][i] = (char)tolower((unsigned char)p[i]);
      saida[k][40] = 0;
      for (int j = 0; j < k && !dup; j++) dup = !strcmp(saida[j], saida[k]);
      if (!dup) k++;
    }
    p = q;
  }
  return k;
}

static const char *base(const char *caminho) {
  const char *b = caminho;
  for (const char *p = caminho; *p; p++) if (*p == '/' || *p == '\\') b = p + 1;
  return b;
}

static int ehVideo(const char *nome) {
  static const char *ext[] = { ".mkv", ".mp4", ".m4v", ".avi", ".mov", ".ts", ".m2ts", ".webm", ".wmv", NULL };
  size_t L = strlen(nome);
  for (int i = 0; ext[i]; i++) {
    size_t e = strlen(ext[i]);
    if (L > e && !strcasecmp(nome + L - e, ext[i])) return 1;
  }
  return 0;
}

// Palavra inteira no nome (sem letra/numero dos dois lados): "nced" casa em
// "[NCED]" e em "NCED01", mas "cm" nao casa em "cm01"? Casa — e o "CM01" dos
// discos de anime e propaganda mesmo. Por isso a lista so tem o que e extra
// em qualquer contexto.
static int temPalavra(const char *nome, const char *w) {
  size_t L = strlen(w);
  for (const char *p = nome; *p; p++) {
    if (strncasecmp(p, w, L)) continue;
    if (p > nome && isalpha((unsigned char)p[-1])) continue;
    if (isalpha((unsigned char)p[L])) continue;
    return 1;
  }
  return 0;
}
static int nomeDeExtra(const char *caminho) {
  static const char *w[] = { "sample", "trailer", "ncop", "nced", "creditless", "menu", "extras",
                             "bonus", "featurette", "featurettes", "preview", "teaser", "promo",
                             "interview", "making", "behind the scenes", "deleted scene",
                             "special features", "cm", "pv", "spot", NULL };
  for (int i = 0; w[i]; i++) if (temPalavra(caminho, w[i])) return 1;
  return 0;
}

// ORDEM NATURAL: "2" antes de "10", caixa ignorada.
static int natural(const char *a, const char *b) {
  while (*a && *b) {
    if (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
      unsigned long long x = 0, y = 0;
      while (isdigit((unsigned char)*a)) x = x * 10 + (unsigned)(*a++ - '0');
      while (isdigit((unsigned char)*b)) y = y * 10 + (unsigned)(*b++ - '0');
      if (x != y) return x < y ? -1 : 1;
      continue;
    }
    int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
    if (ca != cb) return ca < cb ? -1 : 1;
    a++; b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

typedef struct { ParteArq a; char caminho[512]; } ArqTmp;
// As partes primeiro, na ordem natural do caminho; os extras no fim da lista.
static int cmpArq(const void *x, const void *y) {
  const ArqTmp *a = x, *b = y;
  if (a->a.extra != b->a.extra) return a->a.extra - b->a.extra;
  return natural(a->caminho, b->caminho);
}

int partes_ler_item(const char *item, const char *fim, int usenet, PartesLista *l) {
  static ArqTmp tmp[PARTES_MAX];   // so o fio de quem chama usa (um fio de partes por vez)
  static pthread_mutex_t tmpTrava = PTHREAD_MUTEX_INITIALIZER;
  const char *fa, *el;
  int n = 0;
  uint64_t maior = 0;
  if (!item || *item != '{' || !l) return 0;
  if (!fim) fim = js_fim(item);
  memset(l, 0, sizeof *l);
  l->atual = -1;
  l->usenet = usenet;
  l->item = (int)numRaiz(item, fim, "id", -1);
  if (l->item <= 0) return 0;
  fa = raiz(item, fim, "files");
  if (!fa || *fa != '[') return 0;
  pthread_mutex_lock(&tmpTrava);
  el = fa + 1;
  while (el < fim && isspace((unsigned char)*el)) el++;
  for (; el && el < fim && *el == '{' && n < PARTES_MAX; el = js_prox(js_fim(el))) {
    const char *ef = js_fim(el);
    char nome[512] = "", curto[PARTES_NOME] = "";
    double tam, id;
    if (!textoRaiz(el, ef, "name", nome, sizeof nome)) continue;
    if (!textoRaiz(el, ef, "short_name", curto, sizeof curto) || !curto[0])
      snprintf(curto, sizeof curto, "%s", base(nome));
    if (!ehVideo(curto)) continue;
    id = numRaiz(el, ef, "id", -1);
    if (id < 0) continue;
    tam = numRaiz(el, ef, "size", 0);
    memset(&tmp[n], 0, sizeof tmp[n]);
    tmp[n].a.id = (int)id;
    tmp[n].a.bytes = tam > 0 ? (uint64_t)tam : 0;
    snprintf(tmp[n].a.nome, sizeof tmp[n].a.nome, "%s", curto);
    snprintf(tmp[n].caminho, sizeof tmp[n].caminho, "%s", nome);
    tmp[n].a.extra = nomeDeExtra(nome);
    if (tmp[n].a.bytes > maior) maior = tmp[n].a.bytes;
    n++;
  }
  // Bem menor que o maior arquivo (um quinto) tambem e extra: abertura sem
  // creditos, previa, menu. Uma parte de filme nunca e tao menor que a maior.
  // O nome so vale com o tamanho junto: "The Menu (2022).mkv" e o filme, e
  // nenhum extra tem a metade do tamanho do maior arquivo.
  for (int i = 0; i < n; i++) {
    if (maior > 0 && tmp[i].a.bytes * 2 >= maior) tmp[i].a.extra = 0;
    if (maior > 0 && tmp[i].a.bytes > 0 && tmp[i].a.bytes * 5 < maior) tmp[i].a.extra = 1;
  }
  qsort(tmp, (size_t)n, sizeof tmp[0], cmpArq);
  for (int i = 0; i < n; i++) l->arq[i] = tmp[i].a;
  pthread_mutex_unlock(&tmpTrava);
  l->n = n;
  return n > 0;
}

// O arquivo da fonte dentro da lista: pelo nome (e tamanho, quando ha dois
// com o mesmo nome em pastas diferentes); sem nome, o maior.
static int arquivoDaFonte(const PartesLista *l, const char *arquivo, uint64_t bytes) {
  int achou = -1, maior = -1;
  const char *b = arquivo && *arquivo ? base(arquivo) : "";
  for (int i = 0; i < l->n; i++) {
    if (b[0] && !strcasecmp(l->arq[i].nome, b)) {
      if (achou < 0 || (bytes && l->arq[i].bytes == bytes)) achou = i;
    }
    if (maior < 0 || l->arq[i].bytes > l->arq[maior].bytes) maior = i;
  }
  if (achou < 0 && bytes)
    for (int i = 0; i < l->n && achou < 0; i++) if (l->arq[i].bytes == bytes) achou = i;
  return achou >= 0 ? achou : maior;
}

// O item tem um VIDEO com este nome (e, sabendo, este tamanho)?
static int itemTemArquivo(const char *item, const char *fim, const char *b, uint64_t bytes) {
  const char *fa = raiz(item, fim, "files"), *el;
  if (!fa || *fa != '[' || !b[0]) return 0;
  el = fa + 1;
  while (el < fim && isspace((unsigned char)*el)) el++;
  for (; el && el < fim && *el == '{'; el = js_prox(js_fim(el))) {
    const char *ef = js_fim(el);
    char nome[512] = "", curto[PARTES_NOME] = "";
    if (!textoRaiz(el, ef, "name", nome, sizeof nome)) continue;
    if (!textoRaiz(el, ef, "short_name", curto, sizeof curto) || !curto[0])
      snprintf(curto, sizeof curto, "%s", base(nome));
    if (strcasecmp(curto, b)) continue;
    if (bytes) {
      double t = numRaiz(el, ef, "size", 0);
      if (t > 0 && (uint64_t)t != bytes) continue;
    }
    return 1;
  }
  return 0;
}

int partes_achar(const char *corpo, int usenet, char hashes[][41], int nHashes,
                 int hashesConfiaveis, const char *arquivo, uint64_t bytes, PartesLista *l) {
  const char *dados, *it, *b = arquivo && *arquivo ? base(arquivo) : "";
  int um = 0;
  if (!corpo || !l) return 0;
  dados = js_array(corpo, NULL, "data");
  if (!dados) {
    // mylist?id= responde um item so, como objeto
    const char *d = strstr(corpo, "\"data\"");
    if (d) { d = strchr(d + 6, ':'); if (d) { d++; while (isspace((unsigned char)*d)) d++; } }
    if (!d || *d != '{') return 0;
    dados = d; um = 1;
  }
  // 1) e 3) hash: o do addon (confiavel) ou os 40 hex do link.
  for (int pass = 0; pass < 2; pass++) {
    for (it = dados; it && *it == '{'; it = um ? NULL : js_prox(js_fim(it))) {
      const char *f = js_fim(it);
      if (pass == 0) {
        char h[64] = "";
        if (!nHashes || !textoRaiz(it, f, "hash", h, sizeof h) || !h[0]) continue;
        for (int i = 0; i < nHashes; i++) {
          if (strcasecmp(h, hashes[i])) continue;
          if (!partes_ler_item(it, f, usenet, l)) break;
          l->atual = arquivoDaFonte(l, arquivo, bytes);
          snprintf(l->como, sizeof l->como, "%s", i == 0 && hashesConfiaveis ? "hash" : "link hash");
          return 1;
        }
      } else {
        // 2) o nome do arquivo da fonte num arquivo do item.
        if (!b[0] || !itemTemArquivo(it, f, b, bytes)) continue;
        if (!partes_ler_item(it, f, usenet, l)) continue;
        l->atual = arquivoDaFonte(l, arquivo, bytes);
        snprintf(l->como, sizeof l->como, "filename");
        return 1;
      }
    }
  }
  return 0;
}

int partes_principais(const PartesLista *l) {
  int k = 0;
  for (int i = 0; l && i < l->n; i++) k += !l->arq[i].extra;
  return k;
}
int partes_seguinte(const PartesLista *l, int k) {
  for (int i = k + 1; l && i < l->n; i++) if (!l->arq[i].extra) return i;
  return -1;
}
int partes_numero(const PartesLista *l, int k) {
  int num = 0;
  if (!l || k < 0 || k >= l->n || l->arq[k].extra) return 0;
  for (int i = 0; i <= k; i++) num += !l->arq[i].extra;
  return num;
}

int partes_conta_virtual(const PartesLista *l, int k, double pos, double dur, double catDur,
                         double *vpos, double *vdur) {
  double antes = 0, total = 0, este;
  if (!l || k < 0 || k >= l->n || l->arq[k].extra || !(dur > 1.0) || !isfinite(pos)) return 0;
  for (int i = 0; i < l->n; i++) {
    if (l->arq[i].extra) continue;
    total += (double)l->arq[i].bytes;
    if (i < k) antes += (double)l->arq[i].bytes;
  }
  este = (double)l->arq[k].bytes;
  if (!(total > 0) || !(este > 0)) return 0;
  {
    double frac = pos / dur, fim;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    fim = catDur > 1.0 ? catDur : dur * total / este;
    *vdur = fim;
    *vpos = fim * (antes + este * frac) / total;
  }
  return 1;
}

// ---------------------------------------------------------------- busca da lista

typedef struct {
  char url[4096];
  char arquivo[512];
  char hash[48];
  uint64_t bytes;
  unsigned geracao;
} PedidoLista;

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static PartesLista ultima;          // resultado da ultima busca
static unsigned geracaoLista;
static char chaveUltima[160];       // de que fonte e `ultima`

// A FONTE, sem URL na memoria: o rotulo e o nome do arquivo bastam para
// reconhecer a mesma linha da folha entre duas aberturas.
static void chaveFonte(const Stream *s, char *dst, size_t n) {
  unsigned long long h = 1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)s->url; *p; p++) { h ^= *p; h *= 1099511628211ULL; }
  snprintf(dst, n, "%016llx|%.60s|%.60s", h, s->provedor, s->arquivo);
}

static void publicar(unsigned g, const PartesLista *l) {
  pthread_mutex_lock(&trava);
  if (g == geracaoLista) ultima = *l;
  pthread_mutex_unlock(&trava);
}

static void *fioLista(void *u) {
  PedidoLista *pd = u;
  static PartesLista l;   // grande demais para a pilha dos fios do Tizen
  static pthread_mutex_t fioTrava = PTHREAD_MUTEX_INITIALIZER;
  char hashes[9][41];
  int nh = 0, confiavel = 0, disp;
  pthread_mutex_lock(&fioTrava);
  memset(&l, 0, sizeof l);
  l.atual = -1;
  disp = debrid_tb_disponivel();
  if (disp <= 0) {
    l.estado = disp < 0 ? PARTES_SEM_PLANO : PARTES_SEM_CHAVE;
    printf("[partes] files of the source: %s\n",
           disp < 0 ? "the TorBox account has no API plan" : "no TorBox key in Nuvio (Settings > Integrations > Debrid)");
    fflush(stdout);
    publicar(pd->geracao, &l);
    goto fim;
  }
  if (pd->hash[0] && strlen(pd->hash) == 40) {
    for (int i = 0; i < 40; i++) hashes[0][i] = (char)tolower((unsigned char)pd->hash[i]);
    hashes[0][40] = 0;
    nh = 1; confiavel = 1;
  }
  nh += partes_hashes_texto(pd->url, hashes + nh, 9 - nh);
  {
    int achou = 0, st = 0, st2 = 0;
    for (int usenet = 0; usenet < 2 && !achou; usenet++) {
      char *r = debrid_tb_mylist(usenet, 0, usenet ? &st2 : &st);
      if (r) achou = partes_achar(r, usenet, hashes, nh, confiavel, pd->arquivo, pd->bytes, &l);
      free(r);
    }
    if (!achou && confiavel) {
      int id = debrid_tb_adicionar(hashes[0]);
      if (id > 0) {
        char *r = debrid_tb_mylist(0, id, &st);
        if (r && partes_achar(r, 0, hashes, 1, 1, pd->arquivo, pd->bytes, &l)) {
          achou = 1;
          snprintf(l.como, sizeof l.como, "added");
        }
        free(r);
      }
    }
    if (achou) {
      l.estado = PARTES_PRONTO;
      printf("[partes] files of the source: found in TorBox (%s, by %s): %d video file(s), %d part(s); "
             "the source plays file %d\n", l.usenet ? "usenet" : "torrent", l.como, l.n,
             partes_principais(&l), l.atual + 1);
    } else {
      // A lista de torrents nem veio (rede, 4xx/5xx): e erro, nao "nao achou".
      l.estado = st >= 200 && st < 300 ? PARTES_NAO_ACHOU : PARTES_ERRO;
      printf("[partes] files of the source: not found in TorBox (torrents HTTP %d, usenet HTTP %d; "
             "hash from add-on %s, %d hash(es) in the link, filename %s)\n", st, st2,
             confiavel ? "yes" : "no", nh - confiavel, pd->arquivo[0] ? "yes" : "no");
    }
    fflush(stdout);
    publicar(pd->geracao, &l);
  }
fim:
  pthread_mutex_unlock(&fioTrava);
  free(pd);
  return NULL;
}

void partes_pedir(const Stream *s) {
  PedidoLista *pd;
  pthread_t t;
  char ch[160];
  if (!s) return;
  chaveFonte(s, ch, sizeof ch);
  pthread_mutex_lock(&trava);
  if (!strcmp(ch, chaveUltima) && ultima.estado == PARTES_PRONTO) {
    pthread_mutex_unlock(&trava);
    return;   // a mesma fonte, ja achada: a memoria responde
  }
  geracaoLista++;
  memset(&ultima, 0, sizeof ultima);
  ultima.estado = PARTES_BUSCANDO;
  ultima.atual = -1;
  snprintf(chaveUltima, sizeof chaveUltima, "%s", ch);
  pd = calloc(1, sizeof *pd);
  if (pd) {
    snprintf(pd->url, sizeof pd->url, "%s", s->url);
    snprintf(pd->arquivo, sizeof pd->arquivo, "%s", s->arquivo);
    snprintf(pd->hash, sizeof pd->hash, "%s", s->hashTorrent[0] ? s->hashTorrent : s->infoHash);
    pd->bytes = s->tamanhoBytes;
    pd->geracao = geracaoLista;
  }
  pthread_mutex_unlock(&trava);
  if (!pd || pthread_create(&t, NULL, fioLista, pd) != 0) {
    free(pd);
    pthread_mutex_lock(&trava);
    ultima.estado = PARTES_ERRO;
    pthread_mutex_unlock(&trava);
    return;
  }
  pthread_detach(t);
}

int partes_resultado_de(const Stream *s, PartesLista *l) {
  char ch[160];
  int e;
  if (!s) return PARTES_VAZIO;
  chaveFonte(s, ch, sizeof ch);
  pthread_mutex_lock(&trava);
  e = strcmp(ch, chaveUltima) ? PARTES_VAZIO : ultima.estado;
  if (e != PARTES_VAZIO && l) *l = ultima;
  pthread_mutex_unlock(&trava);
  return e;
}

int partes_resultado(PartesLista *l) {
  int e;
  pthread_mutex_lock(&trava);
  if (l) *l = ultima;
  e = ultima.estado;
  pthread_mutex_unlock(&trava);
  return e;
}

void partes_cancelar(void) {
  pthread_mutex_lock(&trava);
  if (ultima.estado == PARTES_BUSCANDO) {
    geracaoLista++;
    memset(&ultima, 0, sizeof ultima);
    chaveUltima[0] = 0;
  }
  pthread_mutex_unlock(&trava);
}

// ---------------------------------------------------------------- link

typedef struct { int usenet, item, arquivo, comLista; unsigned geracao; } PedidoLink;
static unsigned geracaoLink;
static int linkEstado = -2;            // ver partes_link_estado
static char linkUrl[4096];
static PartesLista linkLista;

static void *fioLink(void *u) {
  PedidoLink *pd = u;
  static PartesLista l;
  static char url[4096];
  static pthread_mutex_t fioTrava = PTHREAD_MUTEX_INITIALIZER;
  int ok = 1, st = 0;
  pthread_mutex_lock(&fioTrava);
  memset(&l, 0, sizeof l);
  l.atual = -1;
  url[0] = 0;
  if (pd->comLista) {
    char *r = debrid_tb_mylist(pd->usenet, pd->item, &st);
    const char *d = r ? strstr(r, "\"data\"") : NULL;
    ok = 0;
    if (d) { d = strchr(d + 6, ':'); if (d) { d++; while (isspace((unsigned char)*d)) d++; } }
    if (d && *d == '[') { d++; while (isspace((unsigned char)*d)) d++; }
    if (d && *d == '{' && partes_ler_item(d, js_fim(d), pd->usenet, &l)) {
      for (int i = 0; i < l.n; i++) if (l.arq[i].id == pd->arquivo) { l.atual = i; ok = 1; }
      l.estado = PARTES_PRONTO;
      snprintf(l.como, sizeof l.como, "saved");
    }
    if (!ok) printf("[partes] saved part: the TorBox item or file is gone (HTTP %d)\n", st);
    free(r);
  }
  if (ok) ok = debrid_tb_link(pd->usenet, pd->item, pd->arquivo, url, sizeof url);
  pthread_mutex_lock(&trava);
  if (pd->geracao == geracaoLink) {
    linkEstado = ok ? 1 : -1;
    snprintf(linkUrl, sizeof linkUrl, "%s", ok ? url : "");
    linkLista = l;
  }
  pthread_mutex_unlock(&trava);
  printf("[partes] link for file id %d of TorBox %s %d: %s\n", pd->arquivo,
         pd->usenet ? "usenet" : "torrent", pd->item, ok ? "ready" : "failed");
  fflush(stdout);
  pthread_mutex_unlock(&fioTrava);
  free(pd);
  return NULL;
}

void partes_link_pedir(int usenet, int item, int arquivo, int comLista) {
  PedidoLink *pd = calloc(1, sizeof *pd);
  pthread_t t;
  pthread_mutex_lock(&trava);
  geracaoLink++;
  linkEstado = 0;
  linkUrl[0] = 0;
  if (pd) { pd->usenet = usenet; pd->item = item; pd->arquivo = arquivo; pd->comLista = comLista; pd->geracao = geracaoLink; }
  pthread_mutex_unlock(&trava);
  if (!pd || pthread_create(&t, NULL, fioLink, pd) != 0) {
    free(pd);
    pthread_mutex_lock(&trava);
    linkEstado = -1;
    pthread_mutex_unlock(&trava);
    return;
  }
  pthread_detach(t);
}

int partes_link_estado(char *url, unsigned n, PartesLista *l) {
  int e;
  pthread_mutex_lock(&trava);
  e = linkEstado;
  if (e == 1) {
    if (url && n) snprintf(url, n, "%s", linkUrl);
    if (l) *l = linkLista;
  }
  if (e == 1 || e == -1) { linkEstado = -2; linkUrl[0] = 0; }
  pthread_mutex_unlock(&trava);
  return e;
}

void partes_link_cancelar(void) {
  pthread_mutex_lock(&trava);
  geracaoLink++;
  linkEstado = -2;
  linkUrl[0] = 0;
  pthread_mutex_unlock(&trava);
}

// ---------------------------------------------------------------- sessao

static char sesAlvo[64];
static PartesLista sesLista;
static int sesK = -1;

void partes_sessao_iniciar(const char *alvo, const PartesLista *l, int k) {
  if (!alvo || !*alvo || !l || k < 0 || k >= l->n || l->arq[k].extra || partes_principais(l) < 2) {
    partes_sessao_parar("not a multi-part file");
    return;
  }
  snprintf(sesAlvo, sizeof sesAlvo, "%s", alvo);
  sesLista = *l;
  sesK = k;
  printf("[partes] parts session on: part %d of %d\n", partes_numero(l, k), partes_principais(l));
  fflush(stdout);
}
void partes_sessao_parar(const char *porque) {
  if (sesAlvo[0]) {
    printf("[partes] parts session off: %s\n", porque ? porque : "-");
    fflush(stdout);
  }
  sesAlvo[0] = 0;
  sesK = -1;
}
int partes_sessao_de(const char *alvo) {
  return sesAlvo[0] && alvo && !strcmp(alvo, sesAlvo) && sesK >= 0;
}
const PartesLista *partes_sessao_lista(void) { return sesAlvo[0] ? &sesLista : NULL; }
int partes_sessao_indice(void) { return sesAlvo[0] ? sesK : -1; }
void partes_sessao_ir(int k) { if (sesAlvo[0] && k >= 0 && k < sesLista.n) sesK = k; }

int partes_virtual(const char *alvo, double pos, double dur, double catDur,
                   double *vpos, double *vdur, int *ultima) {
  if (!partes_sessao_de(alvo)) return 0;
  if (!partes_conta_virtual(&sesLista, sesK, pos, dur, catDur, vpos, vdur)) return 0;
  if (ultima) *ultima = partes_seguinte(&sesLista, sesK) < 0;
  return 1;
}

// ---------------------------------------------------------------- ponto (disco)
//
// Uma linha por alvo: "<escopo>\t<alvo>\t<usenet>\t<item>\t<arquivo>\t<pos>\t<quando>".
// Escopo = hash da conta + perfil (a conta nao vai em claro para o disco).
#define LINHA 256
static char pontos[PARTES_GUARDADAS][LINHA];
static int nPontos, pontosLidos;
static pthread_mutex_t pontoTrava = PTHREAD_MUTEX_INITIALIZER;

static void escopo(const char *conta, int perfil, const char *alvo, char *b, size_t t) {
  unsigned long long h = 14695981039346656037ULL;
  for (const unsigned char *p = (const unsigned char *)(conta ? conta : ""); *p; p++) { h ^= *p; h *= 1099511628211ULL; }
  snprintf(b, t, "%016llx-p%d\t%s\t", h, perfil > 0 ? perfil : 0, alvo ? alvo : "");
}
static void pontosLer(void) {
  char *t, *l, *ctx = NULL;
  pontosLidos = 1; nPontos = 0;
  t = dados_ler(PARTES_ARQUIVO);
  if (!t) return;
  for (l = strtok_r(t, "\n", &ctx); l && nPontos < PARTES_GUARDADAS; l = strtok_r(NULL, "\n", &ctx))
    if (strchr(l, '\t') && strlen(l) < LINHA) snprintf(pontos[nPontos++], LINHA, "%s", l);
  free(t);
}
static void pontosGravar(void) {
  size_t tam = (size_t)nPontos * (LINHA + 1) + 1, k = 0;
  char *b = malloc(tam);
  if (!b) return;
  b[0] = 0;
  for (int i = 0; i < nPontos; i++) k += (size_t)snprintf(b + k, tam - k, "%s\n", pontos[i]);
  if (nPontos) dados_gravar(PARTES_ARQUIVO, b);
  else dados_gravar(PARTES_ARQUIVO, "");
  free(b);
}
static int pontoAchar(const char *pre) {
  size_t L = strlen(pre);
  for (int i = 0; i < nPontos; i++) if (!strncmp(pontos[i], pre, L)) return i;
  return -1;
}
static void pontoTirar(int i) {
  memmove(pontos[i], pontos[i + 1], (size_t)(nPontos - i - 1) * LINHA);
  nPontos--;
}

void partes_ponto_guardar(const char *alvo, const char *conta, int perfil, const PartesPonto *p) {
  char pre[160];
  int i;
  if (!alvo || !*alvo || !p || p->item <= 0 || p->arquivo < 0) return;
  escopo(conta, perfil, alvo, pre, sizeof pre);
  pthread_mutex_lock(&pontoTrava);
  if (!pontosLidos) pontosLer();
  if ((i = pontoAchar(pre)) >= 0) pontoTirar(i);
  else if (nPontos == PARTES_GUARDADAS) pontoTirar(0);
  snprintf(pontos[nPontos++], LINHA, "%s%d\t%d\t%d\t%.1f\t%lld", pre, p->usenet ? 1 : 0, p->item,
           p->arquivo, p->pos > 0 && isfinite(p->pos) ? p->pos : 0.0, (long long)time(NULL));
  pontosGravar();
  pthread_mutex_unlock(&pontoTrava);
}

int partes_ponto_pegar(const char *alvo, const char *conta, int perfil, PartesPonto *p) {
  char pre[160];
  int i, ok = 0;
  if (!alvo || !*alvo || !p) return 0;
  escopo(conta, perfil, alvo, pre, sizeof pre);
  pthread_mutex_lock(&pontoTrava);
  if (!pontosLidos) pontosLer();
  if ((i = pontoAchar(pre)) >= 0) {
    int u = 0, it = 0, a = 0;
    double pos = 0;
    long long quando = 0;
    if (sscanf(pontos[i] + strlen(pre), "%d\t%d\t%d\t%lf\t%lld", &u, &it, &a, &pos, &quando) == 5 &&
        it > 0 && a >= 0 && (long long)time(NULL) - quando <= PARTES_VALIDADE_S) {
      p->usenet = u; p->item = it; p->arquivo = a; p->pos = pos;
      ok = 1;
    } else {
      pontoTirar(i);
      pontosGravar();
    }
  }
  pthread_mutex_unlock(&pontoTrava);
  return ok;
}

void partes_ponto_esquecer(const char *alvo, const char *conta, int perfil) {
  char pre[160];
  int i;
  if (!alvo || !*alvo) return;
  escopo(conta, perfil, alvo, pre, sizeof pre);
  pthread_mutex_lock(&pontoTrava);
  if (!pontosLidos) pontosLer();
  if ((i = pontoAchar(pre)) >= 0) { pontoTirar(i); pontosGravar(); }
  pthread_mutex_unlock(&pontoTrava);
}

void partes_ponto_zerar(void) {
  pthread_mutex_lock(&pontoTrava);
  nPontos = 0; pontosLidos = 0;
  pthread_mutex_unlock(&pontoTrava);
}
