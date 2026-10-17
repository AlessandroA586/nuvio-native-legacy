// Text scale (#205): the Home anchors sit at the same REAL px at every scale;
// only text and component sizes change. Measures the Moderna hero copy (left x,
// bottom y), the first row title (x, y) and the right content edge at 85, 100
// and 130 %, inside the same text-scale layer home_desenhar uses, plus the
// trailer video plane (hero/detail full-screen rect) in real screen px.
#include "../src/home.c"
#include <assert.h>
#include <unistd.h>

typedef struct { float heroX, heroBase, rowX, rowY, rowYFoco, dir, vx, vy, vw, vh; } Ancoras;

static Ancoras medir(float s) {
  Ancoras a;
  gfx_escala_txt_definir(s);
  float ant = gfx_escala_txt_entrar();
  float e = gfx_escala();
  a.heroX = homeConteudoX() * e;
  a.heroBase = heroBaseCopia(HOME_LAYOUT_MODERNA, (GfxRect){0}, empurraHero(), 0.0f) * e;
  a.rowX = homeConteudoX() * e;
  a.rowY = (topoFileiras() + empurraHero()) * e;   // focus on the hero
  a.rowYFoco = topoFileiras() * e;                 // focus on the first row
  a.dir = homeConteudoDireita() * e;
  trailer_abrir("t", (GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, 0, 0);
  trailer_rect((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H });
  gfx_escala_sair(ant);
  GfxRect v = trailer_retangulo();   // outside the layer: screen px
  a.vx = v.x; a.vy = v.y; a.vw = v.w; a.vh = v.h;
  return a;
}

int main(void) {
  char dir[] = "/tmp/nv-ancoras-XXXXXX", path[256];
  assert(mkdtemp(dir));
  snprintf(path, sizeof path, "%s/ajustes.txt", dir);
  FILE *f = fopen(path, "w"); assert(f);
  fputs("homeLayoutLocal 0\ncollapseSidebar 1\nmodernSidebar 0\n", f); fclose(f);
  ajustes_dir(dir);
  assert(ajustes_home_layout() == HOME_LAYOUT_MODERNA && ajustes_hero_ligado());
  trailer_teste_falso(1);
  Ancoras ref = medir(1.0f);
  float escalas[] = { 0.85f, 0.9f, 1.15f, 1.3f };
  int mal = 0;
  for (int i = 0; i < 4; i++) {
    Ancoras a = medir(escalas[i]);
    float d[] = { a.heroX - ref.heroX, a.heroBase - ref.heroBase, a.rowX - ref.rowX,
                  a.rowY - ref.rowY, a.rowYFoco - ref.rowYFoco, a.dir - ref.dir,
                  a.vx - ref.vx, a.vy - ref.vy, a.vw - ref.vw, a.vh - ref.vh };
    int ok = 1;
    for (int k = 0; k < 10; k++) if (fabsf(d[k]) > 2.0f) ok = 0;
    printf("%s %3.0f%%: heroX %.1f heroBase %.1f rowX %.1f rowY %.1f rowYFoco %.1f dir %.1f video %.0f,%.0f %.0fx%.0f"
           " (100%%: %.1f %.1f %.1f %.1f %.1f %.1f %.0fx%.0f)\n", ok ? "PASS" : "FAIL", escalas[i] * 100,
           a.heroX, a.heroBase, a.rowX, a.rowY, a.rowYFoco, a.dir, a.vx, a.vy, a.vw, a.vh,
           ref.heroX, ref.heroBase, ref.rowX, ref.rowY, ref.rowYFoco, ref.dir, ref.vw, ref.vh);
    mal += !ok;
  }
  return mal ? 1 : 0;
}
