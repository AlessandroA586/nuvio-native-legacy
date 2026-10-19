#!/usr/bin/env python3
"""Focused regression guards for shared episode info and outline-only focus."""
from pathlib import Path
import re

source = Path('src/detail.c').read_text()
card = source.split('static void desenhaEpisodio(', 1)[1].split('// Abas de informacao:', 1)[0]
checks = {
    'Refined uses Modern episode metrics': 'HOME_LAYOUT_DINAMICA' in source.split('static int epModerna(void)', 1)[1].split('\n', 1)[0],
    'Refined uses the shared title, synopsis, date, rating and progress renderer': 'if (epApple())' not in card,
    'Focus has no plate, glow or thumbnail zoom': not any(s in card for s in ('epPlacaDesenha', 'foco_anel(', 'foco_zoom(', 'gfx_vidro_chip(info')),
    'Artwork has no focus glow or shader border': 'gfx_rect(th, t2, GFX_CARD, 0,' in card,
    'Focus uses a dotted ring without texture': bool(re.search(r'gfx_rect\(r, 0, GFX_ANEL, 0, 2\.0f / r\.h, [^,]+,', source)),
}
for name, passed in checks.items():
    print(('PASS: ' if passed else 'FAIL: ') + name)
if not all(checks.values()):
    raise SystemExit(1)

# Execute production metrics and focus drawing with a recording GL boundary.
import subprocess
import tempfile
metrics = source.split('static int epModerna(void)', 1)[1].split('// Largura do texto', 1)[0]
program = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef struct { float x,y,w,h; } GfxRect;
#define HOME_LAYOUT_MODERNA 0
#define HOME_LAYOUT_DINAMICA 2
#define NV_DETP_EP_RAIO 32.0f
#define NV_DETP_EPM_THUMB_H 360.0f
#define NV_DETP_EP_THUMB_H 384.0f
#define NV_DETP_EP_W 640.0f
#define NV_DETP_EP_GAP 40.0f
#define NV_DETP_EP_PASSO 680.0f
#define NV_DETP_EPM_SIN_DY 62.0f
#define NV_DETP_EPM_SIN_LINHAS 4
#define NV_DETP_EP_LD_SIN 29.0f
#define ANC(x) (x)
#define GFX_ANEL 13
static int layout, size, draws;
static int ajustes_home_layout(void) { return layout; }
static int ajustes_ep_tamanho(void) { return size; }
static float gfx_poster_k(void) { return 1; }
static void gfx_rect(GfxRect r, int tex, int mode, float f, float width,
                     float dots, float radius, float cr,float cg,float cb,float alpha) {
  (void)f; (void)cr; (void)cg; (void)cb;
  assert(tex == 0 && mode == GFX_ANEL); // Any fill/glow fails here.
  assert(width*r.h == (draws ? 2.0f : 4.0f));
  assert(dots > 1 && radius*r.h == NV_DETP_EP_RAIO);
  assert(alpha > 0 && alpha <= 1); draws++;
}
static int epModerna(void)''' + metrics + r'''
int main(void) {
  for(size=0;size<3;size++) {
    layout=HOME_LAYOUT_MODERNA;
    float w=epCardW(), h=epCardH(), thumb=epThumbH(), step=epCardPasso();
    layout=HOME_LAYOUT_DINAMICA;
    assert(epCardW()==w && epCardH()==h && epThumbH()==thumb && epCardPasso()==step);
  }
  GfxRect r={100,100,640,548};
  epFocoDesenha(r,0,1); assert(draws==0);
  epFocoDesenha(r,1,1); assert(draws==2);
  puts("PASS: production metrics match at all sizes; focus draws rings only");
}
'''
with tempfile.TemporaryDirectory(prefix='epinfo-host-') as tmp:
    cfile = Path(tmp) / 'check.c'
    binary = Path(tmp) / 'check'
    cfile.write_text(program)
    subprocess.run(['cc', '-std=c99', str(cfile), '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
