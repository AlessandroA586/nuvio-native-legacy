// Reuse the GPU/GL/storage doubles; exercise the production FPS reporter.
#define main legacy_main
#include "gpunivel.c"
#undef main

static int automatic = 15;
int ajustes_efeitos_automaticos(void) { return automatic; }
static void reports(int n, double fps) {
  while (n--) gpun_relatar(fps, 40, 1000.0 / 60, 1, 1);
}
static void fresh(void) { gravado[0] = 0; gpun_iniciar(1920, 1080); }
int main(void) {
  fresh();
  for (int step = 1; step <= 4; step++) {
    reports(3, 30); assert(gpun_degrau() == step);
    if (step == 2) assert(!strstr(gravado, "foscoVetado=1"));
    reports(1, 34); // Both minimum gains met.
    assert(gpun_efeito_ativo(step));
    assert(gpun_fosco_vetado() == (step >= 2));
  }
  assert(strstr(gravado, "foscoVetado=1"));
  reports(120, 60); assert(gpun_degrau() == 0 && gpun_fosco_vetado());
  gpun_iniciar(1920, 1080); assert(gpun_fosco_vetado());
  gpun_fosco_reiniciar(); assert(!gpun_fosco_vetado());
  gpun_iniciar(1920, 1080); assert(!gpun_fosco_vetado());

  fresh(); reports(3, 30); reports(1, 32);
  assert(gpun_degrau() == 0 && !gpun_efeito_ativo(1));
  reports(3, 30); assert(gpun_degrau() == 2); // No-gain glass skipped.
  reports(1, 30);
  assert(!gpun_fosco_vetado() && !strstr(gravado, "foscoVetado=1"));
  reports(3, 30); assert(gpun_degrau() == 3);
  assert(!gpun_efeito_ativo(1) && !gpun_efeito_ativo(2));

  for (automatic = 0; automatic < 16; automatic++) {
    fresh();
    for (int step = 1; step <= 4; step++) if (automatic & (1 << (step - 1))) {
      reports(3, 30); assert(gpun_degrau() == step); reports(1, 34);
    }
  }
  automatic = 15;
  fresh();
  // Decode/upload and CPU/list/network stalls cannot advance the ladder.
  for (int window = 0; window < 6; window++) {
    for (int f = 0; f < 60; f++) gpun_medir(50, 1, 35, 1, 1);
    reports(1, 20);
  }
  assert(gpun_degrau() == 0);
  // An isolated scroll spike is removed from otherwise healthy steady FPS.
  for (int window = 0; window < 6; window++) {
    for (int f = 0; f < 120; f++) gpun_medir(16.667, 12, 2, 1, 1);
    gpun_medir(200, 180, 2, 1, 1); reports(1, 30);
  }
  assert(gpun_degrau() == 0);
  reports(2, 30); gpun_relatar(30, 1001, 16.667, 1, 1); reports(2, 30);
  assert(gpun_degrau() == 0);
  fresh();
  for (int i = 0; i < 20; i++) gpun_relatar(50, 40, 20, 1, 1);
  assert(gpun_degrau() == 0);
  reports(2, 30); gpun_relatar(30, 40, 16.667, 1, 0); reports(2, 30);
  assert(gpun_degrau() == 0);
  reports(1, 30); reports(1, 34); reports(3, 30); reports(1, 34);
  assert(gpun_fosco_vetado());
  testRenderer = "Different GPU"; gpun_iniciar(1920, 1080);
  assert(!gpun_fosco_vetado());
  puts("gpunivel_escada: ok");
  return 0;
}
