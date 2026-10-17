// Text scale (#205): the Home anchors sit at the same REAL px at every scale;
// only text and component sizes change. Measures the Moderna hero copy (left x,
// bottom y), the first row title (x, y) and the right content edge at 85, 100
// and 130 %, inside the same text-scale layer home_desenhar uses.
#include "../src/home.c"
#include <assert.h>
#include <unistd.h>

typedef struct { float heroX, heroBase, rowX, rowY, rowYFoco, dir; } Ancoras;

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
  gfx_escala_sair(ant);
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
  Ancoras ref = medir(1.0f);
  float escalas[] = { 0.85f, 0.9f, 1.15f, 1.3f };
  int mal = 0;
  for (int i = 0; i < 4; i++) {
    Ancoras a = medir(escalas[i]);
    float d[] = { a.heroX - ref.heroX, a.heroBase - ref.heroBase, a.rowX - ref.rowX,
                  a.rowY - ref.rowY, a.rowYFoco - ref.rowYFoco, a.dir - ref.dir };
    int ok = 1;
    for (int k = 0; k < 6; k++) if (fabsf(d[k]) > 2.0f) ok = 0;
    printf("%s %3.0f%%: heroX %.1f heroBase %.1f rowX %.1f rowY %.1f rowYFoco %.1f dir %.1f"
           " (100%%: %.1f %.1f %.1f %.1f %.1f %.1f)\n", ok ? "PASS" : "FAIL", escalas[i] * 100,
           a.heroX, a.heroBase, a.rowX, a.rowY, a.rowYFoco, a.dir,
           ref.heroX, ref.heroBase, ref.rowX, ref.rowY, ref.rowYFoco, ref.dir);
    mal += !ok;
  }
  return mal ? 1 : 0;
}
