#include "guiafiltro.h"
#include "livetv_regras.h"
#include <stdlib.h>

#define DUP_CAP 16384   /* potencia de 2; acima de 3/4 cheia, para de agrupar */

static void copiar(char *dst, size_t n, const char *s) {
  size_t i = 0;
  for (; s && s[i] && i + 1 < n; i++) dst[i] = (s[i] == '\t' || s[i] == '\r' || s[i] == '\n') ? ' ' : s[i];
  dst[i] = 0;
}

int guiafiltro_cat_oculta(const GuiaFiltro *f, const char *cat) {
  if (!cat) return 0;
  for (int i = 0; i < f->nCat; i++) if (!strcmp(f->cat[i], cat)) return 1;
  return 0;
}
int guiafiltro_cat_alternar(GuiaFiltro *f, const char *cat) {
  if (!cat || !cat[0]) return 0;
  for (int i = 0; i < f->nCat; i++)
    if (!strcmp(f->cat[i], cat)) {
      memmove(f->cat[i], f->cat[i + 1], (size_t)(f->nCat - i - 1) * sizeof f->cat[0]);
      f->nCat--;
      return 0;
    }
  if (f->nCat >= GF_CATS_MAX) return 0;
  copiar(f->cat[f->nCat++], sizeof f->cat[0], cat);
  return 1;
}
int guiafiltro_canal_oculto(const GuiaFiltro *f, const char *id) {
  if (!id) return 0;
  for (int i = 0; i < f->nCan; i++) if (!strcmp(f->can[i].id, id)) return 1;
  return 0;
}
int guiafiltro_canal_alternar(GuiaFiltro *f, const char *id, const char *nome, const char *cat) {
  if (!id || !id[0]) return 0;
  for (int i = 0; i < f->nCan; i++)
    if (!strcmp(f->can[i].id, id)) {
      memmove(&f->can[i], &f->can[i + 1], (size_t)(f->nCan - i - 1) * sizeof f->can[0]);
      f->nCan--;
      return 0;
    }
  if (f->nCan >= GF_CANAIS_MAX) return 0;
  copiar(f->can[f->nCan].id, GF_ID, id);
  copiar(f->can[f->nCan].nome, GF_NOME, nome);
  copiar(f->can[f->nCan].cat, GF_NOME, cat);
  f->nCan++;
  return 1;
}
void guiafiltro_modo_definir(GuiaFiltro *f, int modo) { f->modo = modo > 0 && modo < GF_MODOS ? modo : GF_TODAS; }

void guiafiltro_ler(GuiaFiltro *f, const char *texto) {
  const char *p = texto;
  f->modo = GF_TODAS; f->nCat = 0; f->nCan = 0;
  while (p && *p) {
    const char *fim = strpbrk(p, "\r\n");
    size_t len = fim ? (size_t)(fim - p) : strlen(p);
    if (len > 1 && len < 800) {
      char l[800];
      memcpy(l, p + 1, len - 1);
      l[len - 1] = 0;
      if (*p == 'Q') guiafiltro_modo_definir(f, l[0] - '0');
      else if (*p == 'C') { if (!guiafiltro_cat_oculta(f, l)) guiafiltro_cat_alternar(f, l); }
      else if (*p == 'H') {
        char *nm = strchr(l, '\t'), *ct = NULL;
        if (nm) { *nm++ = 0; ct = strchr(nm, '\t'); if (ct) *ct++ = 0; }
        if (!guiafiltro_canal_oculto(f, l)) guiafiltro_canal_alternar(f, l, nm ? nm : "", ct ? ct : "");
      }
    }
    if (!fim) break;
    p = fim + 1;
  }
}

size_t guiafiltro_texto(const GuiaFiltro *f, char *dst, size_t n) {
  size_t w = 0;
  int i;
  if (!dst || !n) return 0;
  dst[0] = 0;
#define ESCREVE(...) do { int k_ = snprintf(dst + w, n - w, __VA_ARGS__); \
    if (k_ < 0 || (size_t)k_ >= n - w) { dst[0] = 0; return 0; } w += (size_t)k_; } while (0)
  ESCREVE("Q%d\n", f->modo);
  for (i = 0; i < f->nCat; i++) ESCREVE("C%s\n", f->cat[i]);
  for (i = 0; i < f->nCan; i++) ESCREVE("H%s\t%s\t%s\n", f->can[i].id, f->can[i].nome, f->can[i].cat);
#undef ESCREVE
  return w;
}

const char *guiafiltro_arquivo(int perfil, char *dst, size_t n) {
  snprintf(dst, n, "guia-filtro-p%d.txt", perfil > 0 ? perfil : 1);
  return dst;
}

// --- admissao -----------------------------------------------------------------
static uint64_t chaveDe(int fonte, const char *grupo, const char *base) {
  uint64_t h = 1469598103934665603ULL;
  char pre[16];
  const char *p;
  snprintf(pre, sizeof pre, "%d\x1f", fonte);
  for (p = pre; *p; p++) h = (h ^ (unsigned char)*p) * 1099511628211ULL;
  (void)grupo; // Resolution categories do not identify a channel within its source.
  for (p = base; *p; p++) h = (h ^ (unsigned char)*p) * 1099511628211ULL;
  return h ? h : 1;
}
static GuiaFiltroSlot *achar(GuiaFiltroDup *d, uint64_t k) {
  if (!d->t) return NULL;
  for (unsigned i = (unsigned)k & (DUP_CAP - 1);; i = (i + 1) & (DUP_CAP - 1)) {
    if (!d->t[i].chave || d->t[i].chave == k) return &d->t[i];
  }
}

int guiafiltro_avaliar(const GuiaFiltro *f, GuiaFiltroDup *d, int fonte, const char *grupo,
                       const char *id, const char *nome, const char *origem, int *idxVelho) {
  char base[160];
  int res;
  GuiaFiltroSlot *s;
  if (guiafiltro_cat_oculta(f, grupo) || guiafiltro_canal_oculto(f, id)) return GF_FORA;
  if (f->modo == GF_TODAS) return GF_ENTRA;
  res = nv_res_do_texto(nome);
  if (!res) res = nv_res_do_texto(origem);
  if (f->modo == GF_SO_4K) return res >= 2160 ? GF_ENTRA : GF_FORA;
  nv_nome_base(nome, base, sizeof base);
  if (!base[0]) return GF_ENTRA;
  s = achar(d, chaveDe(fonte, grupo, base));
  if (!s || !s->chave) return GF_ENTRA;
  if (idxVelho) *idxVelho = s->idx;
  if (res > s->res) { s->res = res; return GF_TROCA; }
  return GF_FORA_DUP;
}

void guiafiltro_dup_registrar(const GuiaFiltro *f, GuiaFiltroDup *d, int fonte, const char *grupo,
                              const char *nome, const char *origem, int idx) {
  char base[160];
  GuiaFiltroSlot *s;
  uint64_t k;
  if (f->modo != GF_PREFERIR_4K) return;
  nv_nome_base(nome, base, sizeof base);
  if (!base[0]) return;
  if (!d->t) {
    d->t = calloc(DUP_CAP, sizeof *d->t);
    if (!d->t) return;
    d->cap = DUP_CAP; d->n = 0;
  }
  if (d->n >= DUP_CAP / 4 * 3) return;
  k = chaveDe(fonte, grupo, base);
  s = achar(d, k);
  if (!s->chave) d->n++;
  s->chave = k; s->idx = idx; s->res = nv_res_do_texto(nome);
  if (!s->res) s->res = nv_res_do_texto(origem);
}

void guiafiltro_dup_zerar(GuiaFiltroDup *d) { free(d->t); memset(d, 0, sizeof *d); }
