#!/bin/bash
# Painel lateral da colecao (dono, 13/10/2026: "o poster tinha antes, eu so pedi
# para tirar o poster quando a view e de poster"): o cartaz fica em todas as
# exibicoes com painel e some SO na Grade (cartazes). Com logo, cartaz E logo
# (logo abaixo); sem logo, cartaz + titulo. A Vitrine nao usa painel(): o
# destaque dela e logo+meta+sinopse e o cartaz vem na fileira.
set -eu
cd "$(dirname "$0")/.."
python3 -I - <<'PY'
from pathlib import Path
import re, subprocess, tempfile
s = Path('src/vertudo.c').read_text()
start = s.index('static void painel(float a)')
end = s.index('\nstatic const char *retratoLocal', start)
defs = '\n'.join(re.findall(r'^#define (?:VT_TOPO|VT_PAN_W|VT_PAN_X|VT_PAN_Y|VT_PAN_ART_H|VT_PAN_ART_W)\b.*$', s, re.M))
code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned int GLuint;
typedef struct { float x,y,w,h; } GfxRect;
typedef struct { float w,h; } TxtLinha;
typedef struct { char poster[64], backdrop[64], logo[64], titulo[64], genero[32], meta[32], sinopse[64],
  imdb[16], tipo[16], provNome[16]; int nota; } CatItem;
typedef enum { GFX_CARD, GFX_MARCA, GFX_TEXTO } GfxModo;
enum { TXT_HEADLINE, TXT_CAPTION, TXT_HERO_META, TXT_DET_META2 };
#define NV_TELA_W 1920.0f
#define NV_TELA_H 1080.0f
static const void *collection = (void *)1;
static int foco;
enum { VT_LAY_AUTO, VT_LAY_GRADE, VT_LAY_LINHA, VT_LAY_RANKING, VT_LAY_SAGA, VT_LAY_DECADAS, VT_LAY_MURAL, VT_LAY_VITRINE };
static int lay;
float gfx_tex_aspect_atual;
'''+defs+r'''
static CatItem item; static int hasLogo, hasPoster;
static int posterDraws, logoDraws; static float posterY, logoY;
static int viewItem(int i, CatItem *o) { (void)i; *o = item; return 1; }
GLuint tex_obter_logo_larg(const char *u, float w) { (void)u; (void)w; return hasLogo ? 9 : 0; }
GLuint tex_obter_larg(const char *u, float w) { (void)u; (void)w; return hasPoster ? 7 : 0; }
float tex_aspecto(const char *u) { return u[0] ? 2.0f : 0.0f; }
int tex_marca_escura(const char *u) { (void)u; return 0; }
void gfx_cor(GfxRect r, float rd, float a, float b, float c, float d) { (void)r;(void)rd;(void)a;(void)b;(void)c;(void)d; }
void gfx_rect(GfxRect r, GLuint t, GfxModo m, float a, float b, float c, float rd, float e, float f, float g, float h) {
  (void)m;(void)a;(void)b;(void)c;(void)rd;(void)e;(void)f;(void)g;(void)h;
  if (t == 7) { posterDraws++; posterY = r.y; }
  if (t == 9) { logoDraws++; logoY = r.y; }
}
TxtLinha txt_linha_corta(int f, const char *t, int r, int g, int b, int a, float w) { (void)f;(void)t;(void)r;(void)g;(void)b;(void)a;(void)w; return (TxtLinha){10,10}; }
TxtLinha txt_linha(int f, const char *t, int r, int g, int b, int a) { (void)f;(void)t;(void)r;(void)g;(void)b;(void)a; return (TxtLinha){10,10}; }
void txt_desenhar_alpha(TxtLinha l, float x, float y, float a) { (void)l;(void)x;(void)y;(void)a; }
void txt_bloco(int f, const char *t, int r, int g, int b, float x, float y, float w, float lh, float a, int n) { (void)f;(void)t;(void)r;(void)g;(void)b;(void)x;(void)y;(void)w;(void)lh;(void)a;(void)n; }
const char *badges_provedor(const char *n) { return n; }
float badges_desenhar(const char *p, float x, float y, float w, float h, float a) { (void)p;(void)x;(void)y;(void)w;(void)h;(void)a; return 0; }
int imdbnota_obter(const char *i, int n, int s) { (void)i;(void)n;(void)s; return 0; }
void idioma_decimal_texto(char *n, int l) { (void)n;(void)l; }
int ajustes_idioma(void) { return 0; }
'''+s[start:end]+r'''
static void run(int logo, int poster) {
  memset(&item, 0, sizeof item);
  strcpy(item.titulo, "Deadpool"); strcpy(item.poster, "p.jpg");
  if (logo) strcpy(item.logo, "l.png");
  hasLogo = logo; hasPoster = poster; posterDraws = logoDraws = 0;
  painel(1.0f);
}
int main(void) {
  for (lay = VT_LAY_GRADE; lay <= VT_LAY_MURAL; lay++) {
    if (lay == VT_LAY_GRADE) {   // exibicao de cartazes: sem cartaz lateral
      run(1, 1); assert(posterDraws == 0 && logoDraws == 1);
      run(0, 1); assert(posterDraws == 0 && logoDraws == 0);
      continue;
    }
    run(1, 1);   // logo + cartaz: os dois
    assert(posterDraws == 1);
    assert(logoDraws == 1);
    assert(logoY >= posterY + VT_PAN_ART_H);
    run(0, 1);   // sem logo: cartaz + titulo em texto
    assert(posterDraws == 1 && logoDraws == 0);
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src = Path(d)/'painel.c'; binf = Path(d)/'painel'
    src.write_text(code)
    subprocess.run(['cc', str(src), '-o', str(binf), '-lm'], check=True)
    subprocess.run([str(binf)], check=True)
print('vertudo_painel_cartaz: PASS')
PY
