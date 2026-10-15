// MOCK DO SELETOR DE ESTILO DA FILEIRA, REDESENHADO (job 205-estilo2). Dono,
// sobre o 205-fileiraui: "nao mudou nada o componente de estilo de fileira tem
// que redesenhar". A folha escura com lista de texto + previa numa caixa saiu;
// aqui ficam tres direcoes no vocabulario da Home (tvOS 27): foco = pilula de
// vidro branca, material da ilha do relogio, tokens TXT_*, e a fileira REAL
// atras mudando ao vivo (fil_definir_tipo/fil_definir_ordem antes de desenhar).
//   A  doca embaixo: carrossel de mini-previas vivas (home_previa_fileira).
//   B  painel de vidro estreito a direita: miniatura por forma, tamanho dentro.
//   C  sem dialogo: barra de vidro na linha do titulo, com glifos das formas;
//      a propria fileira, em tamanho real, e a previa.
// Estados: redonda (pessoas), tamanho (forma de cartaz com P M G), ordem
// (grupo de colecao). NAO toca src/ctxmenu.c.
//
// NAO ENTRA NA SUITE (*_mock.sh fora do testa-tudo): precisa de janela GL.
#include "ajustes.h"
#include "addons.h"
#include "artehero.h"
#include "catalogo.h"
#include "colecoes.h"
#include "corviva.h"
#include "dados.h"
#include "fileiras.h"
#include "gfx.h"
#include "home.h"
#include "layout.h"
#include "posterprov.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl_compat.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define X0 104.0f
#define DIR (1920.0f - NV_ILHA_MARGEM_D)

extern void ajustes_teste_tema(int tema, int vidro);
static SDL_Window *janela;
static const char *dirDados;

// O que o seletor mostra neste quadro.
typedef struct {
  const char *chave, *titulo;
  FilEstiloLinha lin[FIL_TIPO_N];
  int n, foco, tam;   // linha em foco e o tamanho dela
  int focoTam;        // o foco desceu para o P M G
  int ordem;          // 1 = aba/grupo da ordem em foco (so colecao)
  int ordSel, temOrdem;
  float rowY;         // topo do titulo da fileira na Home (medido)
  float rowB;         // base da fileira
} Sel;
static Sel S;
static void (*desenho)(void);
// A: a Home rola a fileira para cima da doca. No mock, a Home inteira sobe
// `subir` px pelo viewport (o app faria isto pela rolagem dela).
static int subir;

static TxtLinha linha(TxtEstilo e, const char *s, int c, int a) { return txt_linha(e, s, c, c, c, a); }
static void sombra(GfxRect r, float a) {
  gfx_rect((GfxRect){ r.x - 16, r.y - 6, r.w + 32, r.h + 34 }, 0, GFX_SOMBRA, 1, 0, 0, 0.5f, 0, 0, 0, a);
}
// O material da ilha do relogio: sombra curta, vidro .80, luz fraca no canto.
static void ilhaVidro(GfxRect r, float raioPx) {
  float raio = raioPx / r.h;
  sombra(r, 0.34f);
  gfx_vidro_painel(r, raio, 0.86f, 1);
  gfx_luz_canto(r, raio, r.w * 0.25f, -r.h * 0.9f, r.w * 0.85f, 1, 1, 1, 0.07f);
}
static void meio(TxtLinha t, GfxRect r) { txt_desenhar(t, r.x + (r.w - t.w) * 0.5f, r.y + (r.h - t.h) * 0.5f); }

static const char *ORD_ICO[FIL_ORD_N] = { "aj_list-ordered", "aj_calendar", "aj_arrow-down", "aj_shuffle" };
static const char *ORD_AJUDA[FIL_ORD_N] = { "Como a coleção vem", "Mais novos primeiro", "De A a Z", "Embaralhada" };
static const char *TAM_L[3] = { "P", "M", "G" };

// Segmentado de vidro: trilho .62, escolhido branco 14 %, foco = pilula cheia.
static float segmentado(float x, float y, float h, const char **rot, int n, int sel, int foco) {
  float w[8], tot = 8, px;
  int i;
  for (i = 0; i < n; i++) { w[i] = fmaxf(h, linha(TXT_AJ_SEG, rot[i], 235, 255).w + 36); tot += w[i]; }
  gfx_vidro_painel((GfxRect){ x, y, tot, h }, 0.5f, 0.62f, 1);
  for (px = x + 4, i = 0; i < n; px += w[i], i++) {
    GfxRect s = { px, y + 4, w[i], h - 8 };
    float f = i == foco ? 1.0f : 0.0f;
    if (f > 0) gfx_vidro_pilula_cheia(s, 0.5f, 1, 1);
    else if (i == sel) gfx_cor(s, 0.5f, 1, 1, 1, 0.14f);
    meio(linha(TXT_AJ_SEG, rot[i], f > 0 ? gfx_vidro_tinta(1) : (i == sel ? 242 : 160), 255), s);
  }
  return tot;
}

static const char *nomeLinha(int i) { return S.lin[i].rotulo; }
static const char *nomeFoco(void) {
  const FilEstiloLinha *l = &S.lin[S.foco];
  return l->n > 1 ? l->nomes[S.tam] : nomeLinha(S.foco);
}
static int tipoDe(int i, int t) { return S.lin[i].tipos[S.lin[i].n > 1 ? t : 0]; }
static int refDe(int i) { return S.lin[i].tipos[S.lin[i].n - 1]; }
static void dica(const char *s, float x, float y, float a) {
  txt_desenhar_alpha(linha(TXT_ILHA_SUB, s, 225, 255), x, y, a);
}

// ---------------------------------------------------------------- A: doca
// A fileira sobe para cima da doca (a Home rola; aqui ja vem rolada) e a doca
// mora no rodape: carrossel de mini-previas vivas, a em foco maior e branca.
#define A_Y 840.0f
#define A_H 208.0f
static void desenhaA(void) {
  GfxRect d = { 64, A_Y, 1792, A_H };
  float tw = 256, th = 128, gap = 18, x, y = A_Y + 62;
  int i, k0;
  gfx_rect((GfxRect){ 0, A_Y - 140, 1920, 1080 - A_Y + 140 }, 0, GFX_VEU_BAIXO, 0, 0, 0, 0, 0, 0, 0, 1.0f);
  ilhaVidro(d, 36);
  // Cabecalho da doca: o que esta sendo editado + Estilo|Ordem (so colecao).
  if (S.temOrdem) {
    const char *ab[2] = { "Estilo", "Ordem" };
    float w = segmentado(d.x + 28, A_Y + 14, 44, ab, 2, S.ordem, S.ordem);
    txt_desenhar(linha(TXT_ILHA_NOME, S.titulo, 242, 255), d.x + 28 + w + 20, A_Y + 22);
  } else {
    TxtLinha eb = linha(TXT_ILHA_SECAO, "ESTILO DA FILEIRA", 170, 255);
    txt_desenhar(eb, d.x + 32, A_Y + 26);
    txt_desenhar(linha(TXT_ILHA_NOME, S.titulo, 242, 255), d.x + 32 + eb.w + 16, A_Y + 22);
  }
  dica(S.ordem ? "◀ ▶ ordem   ·   OK aplicar   ·   Voltar cancelar"
       : S.lin[S.foco].n > 1 ? "◀ ▶ estilo   ·   ▲ tamanho   ·   OK aplicar   ·   Voltar cancelar"
                             : "◀ ▶ estilo   ·   OK aplicar   ·   Voltar cancelar",
       d.x + d.w - 560, A_Y + 26, 0.62f);
  if (S.ordem) {
    float w = 404;
    for (i = 0; i < FIL_ORD_N; i++) {
      GfxRect t = { d.x + 28 + i * (w + gap), y, w, th };
      int f = i == S.ordSel;
      if (f) { sombra(t, 0.45f); gfx_vidro_pilula_cheia(t, 22 / th, 1, 1); }
      else gfx_cor(t, 22 / th, 1, 1, 1, 0.06f);
      { float c = f ? gfx_vidro_tinta(1) / 255.0f : 0.92f;
        gfx_icone((GfxRect){ t.x + 24, t.y + 26, 34, 34 }, ORD_ICO[i], c, c, c, 1); }
      txt_desenhar(linha(TXT_ILHA_ITEM, fil_ordem_rotulo(i), f ? gfx_vidro_tinta(1) : 238, 255), t.x + 24, t.y + 76);
      txt_desenhar(linha(TXT_ILHA_APOIO, ORD_AJUDA[i], f ? 70 : 170, 255), t.x + 24, t.y + 106);
      if (i == fil_ordem(S.chave) && !f) gfx_icone((GfxRect){ t.x + t.w - 46, t.y + 22, 26, 26 }, "aj_check", 1, 1, 1, 0.8f);
    }
    return;
  }
  // Carrossel: a em foco fica na segunda posicao, as de fora somem na borda.
  k0 = S.foco - 1 < S.n - 6 ? S.foco - 1 : S.n - 6;
  if (k0 < 0) k0 = 0;
  gfx_recorte(d.x + 2, A_Y + 40, d.w - 4, A_H - 42);
  for (x = d.x + 28, i = k0; i < S.n && x < d.x + d.w; i++) {
    int f = i == S.foco;
    float s = f ? 1.10f : 1.0f, w = tw * s, h = th * s;
    GfxRect t = { x, y + (th - h) * 0.5f - (f ? 6 : 0), w, h };
    GfxRect pv = { t.x + 12, t.y + 12, t.w - 24, t.h - 54 };
    float a = x + w > d.x + d.w - 120 ? 0.45f : 1.0f;
    if (f) { sombra(t, 0.5f); gfx_vidro_pilula_cheia(t, 22 / h, 1, 1); }
    else gfx_cor(t, 22 / h, 1, 1, 1, 0.06f * a);
    gfx_cor(pv, 12 / pv.h, 0, 0, 0, f ? 0.88f : 0.42f * a);
    home_previa_fileira(S.chave, f ? tipoDe(i, S.tam) : tipoDe(i, S.lin[i].n - 1),
                        f ? refDe(i) : tipoDe(i, S.lin[i].n - 1),
                        (GfxRect){ pv.x + 8, pv.y + 8, pv.w - 16, pv.h - 16 }, a);
    gfx_recorte(d.x + 2, A_Y + 40, d.w - 4, A_H - 42);
    txt_desenhar_alpha(linha(TXT_ILHA_ITEM, nomeLinha(i), f ? gfx_vidro_tinta(1) : 232, 255),
                       t.x + 16, t.y + t.h - 36, a);
    x += w + gap;
  }
  gfx_sem_recorte();
  // P M G so na forma em foco: uma capsula que pousa na borda de cima da doca.
  if (S.lin[S.foco].n > 1) {
    float xf = d.x + 28 + (S.foco - k0) * (tw + gap);
    segmentado(xf + tw * 1.10f - 180, A_Y - 26, 52, TAM_L, 3, S.tam, S.focoTam ? S.tam : -1);
  }
}

// ---------------------------------------------------------------- B: painel
// Painel estreito a direita, no material do relogio; a fileira fica viva a
// esquerda (os primeiros quatro cards nunca ficam cobertos).
#define B_X 1360.0f
static void desenhaB(void) {
  GfxRect p = { B_X, NV_ILHA_Y, DIR - B_X, 1044 - NV_ILHA_Y };
  float y, ih = 116, x = p.x + 20, w = p.w - 40;
  int i, k0;
  gfx_rect((GfxRect){ B_X - 260, 0, 1920 - B_X + 260, 1080 }, 0, GFX_VEU_CSS, 0, 0, 0, 1, 0, 0, 0, 0.0f);
  ilhaVidro(p, 36);
  txt_desenhar(linha(TXT_ILHA_SECAO, "ESTILO DA FILEIRA", 170, 255), x + 12, p.y + 28);
  txt_desenhar(linha(TXT_ILHA_TITULO, S.titulo, 242, 255), x + 12, p.y + 58);
  y = p.y + 128;
  if (S.temOrdem) {
    const char *ab[2] = { "Estilo", "Ordem" };
    segmentado(x + 8, y, 48, ab, 2, S.ordem, -1);
    y += 68;
  }
  if (S.ordem) {
    for (i = 0; i < FIL_ORD_N; i++, y += 96) {
      GfxRect r = { x, y, w, 84 };
      int f = i == S.ordSel;
      float c = f ? gfx_vidro_tinta(1) / 255.0f : 0.92f;
      if (f) gfx_vidro_pilula_cheia(r, 22 / r.h, 1, 1);
      gfx_cor((GfxRect){ r.x + 14, r.y + 14, 56, 56 }, 0.5f, f ? 0 : 1, f ? 0 : 1, f ? 0 : 1, f ? 0.08f : 0.08f);
      gfx_icone((GfxRect){ r.x + 28, r.y + 28, 28, 28 }, ORD_ICO[i], c, c, c, 1);
      txt_desenhar(linha(TXT_ILHA_ITEM, fil_ordem_rotulo(i), f ? gfx_vidro_tinta(1) : 238, 255), r.x + 88, r.y + 14);
      txt_desenhar(linha(TXT_ILHA_APOIO, ORD_AJUDA[i], f ? 70 : 165, 255), r.x + 88, r.y + 46);
      if (i == fil_ordem(S.chave)) gfx_icone((GfxRect){ r.x + r.w - 44, r.y + 29, 26, 26 }, "aj_check", c, c, c, 0.85f);
    }
    dica("▲ ▼ ordem  ·  OK aplicar  ·  Voltar cancelar", x + 12, p.y + p.h - 46, 0.6f);
    return;
  }
  k0 = S.foco > 3 ? S.foco - 3 : 0;
  { float y0 = y - 6;
#define B_CLIP() gfx_recorte(p.x, y0, p.w, p.y + p.h - 84 - y0)
  B_CLIP();
  for (i = k0; i < S.n; i++) {
    int f = i == S.foco, comTam = f && S.lin[i].n > 1;
    GfxRect r = { x, y, w, comTam ? ih + 64 : ih - 12 };
    if (r.y + r.h > p.y + p.h - 84) break;
    GfxRect th = { r.x + 12, r.y + 12, 168, 80 };
    int ink = f && !S.focoTam ? gfx_vidro_tinta(1) : 238;
    if (f && !S.focoTam) gfx_vidro_pilula_cheia(r, 24 / r.h, 1, 1);
    else if (f) gfx_cor(r, 24 / r.h, 1, 1, 1, 0.14f);
    gfx_cor(th, 12 / th.h, 0, 0, 0, 0.85f);
    home_previa_fileira(S.chave, f ? tipoDe(i, S.tam) : tipoDe(i, S.lin[i].n - 1),
                        f ? refDe(i) : tipoDe(i, S.lin[i].n - 1),
                        (GfxRect){ th.x + 6, th.y + 6, th.w - 12, th.h - 12 }, 1);
    B_CLIP();
    txt_desenhar(linha(TXT_ILHA_ITEM, f ? nomeFoco() : nomeLinha(i), ink, 255), th.x + th.w + 18, r.y + 22);
    if (S.lin[i].n > 1 && !comTam)
      txt_desenhar(linha(TXT_ILHA_APOIO, "3 tamanhos", f ? 70 : 150, 255), th.x + th.w + 18, r.y + 54);
    if (comTam) segmentado(th.x, r.y + ih - 6, 52, TAM_L, 3, S.tam, S.focoTam ? S.tam : -1);
    y += r.h + 12;
  }
#undef B_CLIP
  }
  gfx_sem_recorte();
  dica(S.focoTam ? "◀ ▶ tamanho  ·  OK aplicar  ·  Voltar cancelar"
                 : "▲ ▼ estilo  ·  OK aplicar  ·  Voltar cancelar", x + 12, p.y + p.h - 46, 0.6f);
}

// ---------------------------------------------------------------- C: barra
// Sem dialogo. A Home escurece fora da faixa da fileira; na linha do titulo
// dela pousa uma barra de vidro com um GLIFO por forma (desenhado, nao arte).
// A forma em foco abre em pilula branca com o nome; P M G vem logo depois.
static void glifo(int tipo, GfxRect b, float c, float a) {
  float cx = b.x + b.w * 0.5f, cy = b.y + b.h * 0.5f;
  int i;
#define Q(x, y, w, h, r) gfx_cor((GfxRect){ (x), (y), (w), (h) }, (r), c, c, c, a)
  switch (tipo) {
    case FIL_TIPO_REDONDA:
      for (i = -1; i <= 1; i++) Q(cx + i * 13 - 5.5f, cy - 5.5f, 11, 11, 0.5f);
      break;
    case FIL_TIPO_CARTAZ:
      for (i = -1; i <= 1; i++) Q(cx + i * 11 - 4.5f, cy - 7, 9, 14, 0.2f);
      break;
    case FIL_TIPO_POSTER_GRANDE: case FIL_TIPO_FICHA:
      Q(cx - 13, cy - 10, 14, 20, 0.15f);
      for (i = 0; i < 3; i++) Q(cx + 4, cy - 8 + i * 6, i == 2 ? 6 : 10, 3, 0.5f);
      break;
    case FIL_TIPO_TOP10: case FIL_TIPO_RANKING:
      Q(cx - 13, cy - 9, 4, 18, 0.4f);
      Q(cx - 5, cy - 9, 12, 18, 0.15f);
      Q(cx + 10, cy - 9, 5, 18, 0.15f);
      break;
    case FIL_TIPO_PALCO:
      Q(cx - 7, cy - 10, 14, 20, 0.15f);
      Q(cx - 15, cy - 6, 6, 12, 0.2f);
      Q(cx + 9, cy - 6, 6, 12, 0.2f);
      break;
    case FIL_TIPO_LINHA_TEMPO:
      Q(cx - 15, cy + 6, 30, 2, 0.5f);
      for (i = -1; i <= 1; i++) Q(cx + i * 11 - 4, cy - 9, 8, 12, 0.2f);
      break;
    case FIL_TIPO_HEROI:
      Q(cx - 15, cy - 9, 30, 18, 0.15f);
      break;
    case FIL_TIPO_DESTAQUE_QUADRADO: case FIL_TIPO_DESTAQUE_QUADRADO_M: case FIL_TIPO_DESTAQUE_QUADRADO_G:
      Q(cx - 15, cy - 6, 16, 12, 0.15f);
      Q(cx + 3, cy - 6, 12, 12, 0.15f);
      break;
    case FIL_TIPO_LARGA:
      Q(cx - 15, cy - 6, 20, 12, 0.15f);
      Q(cx + 7, cy - 6, 8, 12, 0.15f);
      break;
    default:   // paisagens
      Q(cx - 15, cy - 5, 18, 10, 0.15f);
      Q(cx + 5, cy - 5, 10, 10, 0.15f);
      break;
  }
#undef Q
}
static void desenhaC(void) {
  float h = NV_ILHA_H_ABERTA, y = S.rowY - 10, x, chip = 56;
  float tituloW = linha(TXT_ROW_TITULO, S.titulo, 242, 255).w;
  GfxRect bar;
  int i;
  // Fora da faixa da fileira, a Home recua (sem vidro: dois retangulos).
  gfx_cor((GfxRect){ 0, 0, 1920, y - 14 }, 0, 0, 0, 0, 0.66f);
  gfx_cor((GfxRect){ 0, S.rowB, 1920, 1080 - S.rowB }, 0, 0, 0, 0, 0.66f);
  x = X0 + tituloW + 28;
  if (S.ordem) {
    float w = 8;
    for (i = 0; i < FIL_ORD_N; i++)
      w += i == S.ordSel ? 70 + linha(TXT_AJ_SEG, fil_ordem_rotulo(i), 0, 255).w : chip;
    bar = (GfxRect){ x, y, w, h };
  } else {
    float w = 8;
    for (i = 0; i < S.n; i++) w += i == S.foco ? 70 + linha(TXT_AJ_SEG, nomeFoco(), 0, 255).w : chip;
    bar = (GfxRect){ x, y, w, h };
  }
  sombra(bar, 0.4f);
  gfx_vidro_painel(bar, 0.5f, 0.80f, 1);
  gfx_luz_canto(bar, 0.5f, bar.w * 0.25f, -bar.h * 0.9f, bar.w * 0.85f, 1, 1, 1, 0.07f);
  x = bar.x + 4;
  if (S.ordem) {
    for (i = 0; i < FIL_ORD_N; i++) {
      int f = i == S.ordSel;
      TxtLinha t = linha(TXT_AJ_SEG, fil_ordem_rotulo(i), gfx_vidro_tinta(1), 255);
      float w = f ? 70 + t.w : chip, c = f ? gfx_vidro_tinta(1) / 255.0f : 0.80f;
      GfxRect s = { x, y + 4, w, h - 8 };
      if (f) gfx_vidro_pilula_cheia(s, 0.5f, 1, 1);
      gfx_icone((GfxRect){ s.x + 16, s.y + (s.h - 24) * 0.5f, 24, 24 }, ORD_ICO[i], c, c, c, 1);
      if (f) txt_desenhar(t, s.x + 50, s.y + (s.h - t.h) * 0.5f);
      x += w;
    }
  } else {
    for (i = 0; i < S.n; i++) {
      int f = i == S.foco;
      TxtLinha t = linha(TXT_AJ_SEG, nomeFoco(), gfx_vidro_tinta(1), 255);
      float w = f ? 70 + t.w : chip, c = f && !S.focoTam ? gfx_vidro_tinta(1) / 255.0f : 0.80f;
      GfxRect s = { x, y + 4, w, h - 8 };
      if (f && !S.focoTam) gfx_vidro_pilula_cheia(s, 0.5f, 1, 1);
      else if (f) gfx_cor(s, 0.5f, 1, 1, 1, 0.14f);
      glifo(S.lin[i].tipos[0], (GfxRect){ s.x + 10, s.y, 36, s.h }, c, 1);
      if (f) txt_desenhar(f && !S.focoTam ? t : linha(TXT_AJ_SEG, nomeFoco(), 242, 255),
                          s.x + 52, s.y + (s.h - t.h) * 0.5f);
      x += w;
    }
    if (S.lin[S.foco].n > 1)
      segmentado(bar.x + bar.w + 12, y + 6, h - 12, TAM_L, 3, S.tam, S.focoTam ? S.tam : -1);
  }
  dica(S.ordem ? "◀ ▶ ordem   ·   ▲ estilo   ·   OK aplicar   ·   Voltar cancelar"
       : S.focoTam ? "◀ ▶ tamanho   ·   ▲ estilo   ·   OK aplicar   ·   Voltar cancelar"
       : S.temOrdem ? "◀ ▶ estilo   ·   ▼ ordem   ·   OK aplicar   ·   Voltar cancelar"
                    : "◀ ▶ estilo   ·   OK aplicar   ·   Voltar cancelar",
       X0, S.rowB + 12, 0.7f);
}

// ---------------------------------------------------------------- harness
static void gravar(const char *bmp) {
  unsigned char *pix = malloc(1920 * 1080 * 4);
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
  int y;
  assert(pix && s);
  glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
  for (y = 0; y < 1080; y++)
    memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
  assert(SDL_SaveBMP(s, bmp) == 0);
  SDL_FreeSurface(s);
  free(pix);
}

static void quadros(int n, const char *bmp) {
  int i;
  for (i = 0; i < n; i++) {
    Uint32 agora = SDL_GetTicks();
    SDL_PumpEvents();
    tex_bombear(8);
    home_atualizar(1.0f / 60.0f, agora);
    corviva_quadro(1.0f / 60.0f, ajustes_cor_viva(), ajustes_cor_logo(), ajustes_animacoes_reduzidas());
    txt_novo_quadro();
    tex_novo_quadro();
    gfx_novo_quadro();
    gfx_ambiente_preparar();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gfx_ambiente(1.0f);
    if (subir) glViewport(0, subir, 1920, 1080);
    home_desenhar(agora);
    glViewport(0, 0, 1920, 1080);
    if (desenho) desenho();
    if (bmp && i == n - 1) gravar(bmp);
    SDL_GL_SwapWindow(janela);
    SDL_Delay(4);
  }
}

static void teclaHome(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  home_evento(&e);
  e.type = SDL_KEYUP;
  home_evento(&e);
}

static void focaFileira(const char *titulo) {
  int i;
  for (i = 0; i < 14; i++) teclaHome(SDLK_UP);
  for (i = 0; i < 12 && !strstr(home_rastro_foco(), titulo); i++) {
    teclaHome(SDLK_DOWN);
    quadros(4, NULL);
  }
  assert(strstr(home_rastro_foco(), titulo));
}

static const char *JS =
  "{\"collections\":[{\"id\":\"cs\",\"title\":\"Streaming\","
  "\"backdropImageUrl\":\"deploy/app/art/07.jpg\",\"folders\":["
  "{\"id\":\"n\",\"title\":\"Netflix\",\"coverImageUrl\":\"deploy/app/art/03.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
  "{\"id\":\"h\",\"title\":\"HBO Max\",\"coverImageUrl\":\"deploy/app/art/05.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
  "{\"id\":\"d\",\"title\":\"Disney+\",\"coverImageUrl\":\"deploy/app/art/09.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
  "{\"id\":\"a\",\"title\":\"Apple TV+\",\"coverImageUrl\":\"deploy/app/art/12.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]},"
  "{\"id\":\"p\",\"title\":\"Prime Video\",\"coverImageUrl\":\"deploy/app/art/15.jpg\",\"tileShape\":\"LANDSCAPE\","
  "\"sources\":[{\"addonBaseUrl\":\"https://addon.invalid/x\",\"type\":\"movie\",\"catalogId\":\"k\"}]}]}]}";

#define PESSOAS "tmdb.people_popular"
#define FILMES "tmdb.trending_movie"
#define COLECAO "collection_cs"
static const char *NOMES[] = { "Greta Gerwig", "Denis Villeneuve", "Bong Joon-ho",
  "Ava DuVernay", "Wes Anderson", "Martin Scorsese", "Chloé Zhao", "Jordan Peele" };
static const char *FILMES_T[] = { "Duna: Parte Dois", "Pobres Criaturas", "Oppenheimer",
  "Anatomia de uma Queda", "Vidas Passadas", "Zona de Interesse", "Furiosa", "Guerra Civil" };

static void preparaSel(const char *chave, const char *titulo, int tipoFoco, int tam) {
  int k;
  memset(&S, 0, sizeof S);
  S.chave = chave; S.titulo = titulo;
  S.n = fil_estilo_linhas(chave, S.lin, FIL_TIPO_N);
  assert(S.n > 0);
  S.foco = -1;
  for (k = 0; k < S.n; k++) if (S.lin[k].tipos[0] == tipoFoco) S.foco = k;
  assert(S.foco >= 0);
  S.tam = S.lin[S.foco].n > 1 ? tam : 0;
  S.temOrdem = !strncmp(chave, "collection_", 11);
  // Ao vivo: a fileira real ja desenha a forma em foco.
  fil_definir_tipo(chave, tipoDe(S.foco, S.tam));
}

// Faixa da fileira em foco na Home (medida na captura de referencia).
static void faixa(float topo, float base) { S.rowY = topo; S.rowB = base; }

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/Volumes/ExternalSSD/tmp/rev/estilo2";
  static CatItem itens[20];
  static CatFileira fils[3];
  static void (*dirs[3])(void) = { desenhaA, desenhaB, desenhaC };
  SDL_GLContext gl;
  char bmp[800], cache[700];
  int i, d;

  dados_iniciar("deploy/app/art");
  dirDados = dados_dir();
  if (!dirDados || !strstr(dirDados, "nuvio-estilo2-dados")) {
    fprintf(stderr, "recuse: NUVIO_DADOS tem de ser a pasta temporaria do teste\n");
    return 1;
  }
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  janela = SDL_CreateWindow("Nuvio: estilo2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(janela);
  gl = SDL_GL_CreateContext(janela);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(192);
  gfx_borrao_iniciar(480, 270);
  artehero_definir_falhou(tex_falhou);
  snprintf(cache, sizeof cache, "%s/cache", dirDados);
  tex_cache_dir(cache);
  gfx_icones_dir("deploy/app/art");
  { FILE *a;
    snprintf(cache, sizeof cache, "%s/ajustes.txt", dirDados);
    a = fopen(cache, "w");
    assert(a);
    fprintf(a, "idioma 0\nhomeLayoutLocal 0\nvidroLocal 1\n"
               "modernLandscapePostersEnabled 1\nselected_theme 0\n");
    fclose(a);
    ajustes_dir(dirDados);
    ajustes_teste_tema(0, 1);   // realce Branco: o foco e a pilula BRANCA da Home
    artehero_fundo_addon(ajustes_fundo_addon());
    posterprov_preferir_addon(ajustes_poster_addon());
    col_arte_conta(ajustes_col_arte_conta()); }
  assert(home_iniciar("deploy/app/art"));

  { AddonRemoto addon = { .nome = "Fixture", .url = "http://127.0.0.1:1/manifest.json", .ativo = 1 };
    assert(addons_definir_lista(&addon, 1)); }
  memset(itens, 0, sizeof itens);
  memset(fils, 0, sizeof fils);
  snprintf(fils[0].chave, sizeof fils[0].chave, "continue_watching");
  snprintf(fils[0].titulo, sizeof fils[0].titulo, "Continuar assistindo");
  snprintf(fils[0].tipo, sizeof fils[0].tipo, "movie");
  fils[0].ini = 0; fils[0].n = 4;
  snprintf(fils[1].chave, sizeof fils[1].chave, FILMES);
  snprintf(fils[1].titulo, sizeof fils[1].titulo, "Em alta");
  snprintf(fils[1].tipo, sizeof fils[1].tipo, "movie");
  snprintf(fils[1].base, sizeof fils[1].base, "https://addon.invalid/x");
  snprintf(fils[1].catId, sizeof fils[1].catId, "trending_movie");
  fils[1].ini = 4; fils[1].n = 8;
  snprintf(fils[2].chave, sizeof fils[2].chave, PESSOAS);
  snprintf(fils[2].titulo, sizeof fils[2].titulo, "Diretores populares");
  snprintf(fils[2].tipo, sizeof fils[2].tipo, "movie");
  snprintf(fils[2].base, sizeof fils[2].base, "https://addon.invalid/x");
  snprintf(fils[2].catId, sizeof fils[2].catId, "people_popular");
  fils[2].ini = 12; fils[2].n = 8;
  for (i = 0; i < 20; i++) {
    CatItem *c = &itens[i];
    snprintf(c->imdb, sizeof c->imdb, "tt90%03d", i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    if (i < 4) {
      snprintf(c->titulo, sizeof c->titulo, "Titulo Fixture %d", i);
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", i);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", i);
      c->progresso = 15 + i * 20;
    } else if (i < 12) {
      snprintf(c->titulo, sizeof c->titulo, "%s", FILMES_T[i - 4]);
      snprintf(c->backdrop, sizeof c->backdrop, "deploy/app/art/%02d.jpg", 16 + i);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/poster/%02d.jpg", 16 + i);
    } else {
      snprintf(c->titulo, sizeof c->titulo, "%s", NOMES[i - 12]);
      snprintf(c->poster, sizeof c->poster, "deploy/app/art/elenco/%02d_%d.jpg", (i - 12) / 6, (i - 12) % 6);
      snprintf(c->backdrop, sizeof c->backdrop, "%s", c->poster);
    }
  }
  assert(col_definir_json(JS) == 5);
  cat_definir_tudo(itens, 20, fils, 3);
  quadros(60, NULL);

  for (d = 0; d < 3; d++) {
    const char L = "ABC"[d];
    // 1. Pessoas, foco na Redonda.
    desenho = NULL;
    preparaSel(PESSOAS, "Diretores populares", FIL_TIPO_REDONDA, 0);
    focaFileira("Diretores populares");
    teclaHome(SDLK_RIGHT);
    faixa(518, 840);
    desenho = dirs[d];
    quadros(70, NULL);
    snprintf(bmp, sizeof bmp, "%s/%c-redonda.bmp", saida, L);
    quadros(1, bmp);
    // 2. Filmes, foco numa forma com tamanhos (a primeira de tres), em M.
    desenho = NULL;
    { FilEstiloLinha l[FIL_TIPO_N]; int n = fil_estilo_linhas(FILMES, l, FIL_TIPO_N), k, t = -1;
      for (k = 0; k < n && t < 0; k++) if (l[k].n == 3) t = l[k].tipos[0];
      assert(t >= 0);
      preparaSel(FILMES, "Em alta", t, 1); }
    S.focoTam = d != 0;
    focaFileira("Em alta");
    faixa(518, 868);
    subir = d == 0 ? 64 : 0;
    desenho = dirs[d];
    quadros(70, NULL);
    snprintf(bmp, sizeof bmp, "%s/%c-tamanho.bmp", saida, L);
    quadros(1, bmp);
    subir = 0;
    // 3. Grupo de colecao, foco na Ordem (A-Z: a fileira reordena ao vivo).
    desenho = NULL;
    preparaSel(COLECAO, "Streaming", FIL_TIPO_AUTO, 0);
    S.ordem = 1; S.ordSel = FIL_ORD_AZ;
    fil_definir_ordem(COLECAO, FIL_ORD_AZ);
    focaFileira("Streaming");
    faixa(518, 860);
    desenho = dirs[d];
    quadros(70, NULL);
    snprintf(bmp, sizeof bmp, "%s/%c-ordem.bmp", saida, L);
    quadros(1, bmp);
    fil_definir_ordem(COLECAO, FIL_ORD_LISTA);
  }
  printf("[estilo2] ok\n");
  SDL_Quit();
  return 0;
}
