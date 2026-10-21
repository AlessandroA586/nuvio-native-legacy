// Reuse the GPU/GL/storage doubles; exercise the production FPS reporter.
#define main legacy_main
#include "gpunivel.c"
#undef main

static int automatic = 15;
int ajustes_efeitos_automaticos(void) { return automatic; }
static void reports(int n, double fps) {
  while (n--) gpun_relatar(fps, 40, 1000.0 / 60, 1, 1);
}
int main(void) {
  gravado[0] = 0;
  gpun_iniciar(1920, 1080);
  for (int step = 1; step <= 4; step++) {
    reports(2, 30); assert(gpun_degrau() == step - 1);
    reports(1, 30); assert(gpun_degrau() == step);
    assert(gpun_fosco_vetado() == (step >= 2));
  }
  assert(strstr(gravado, "foscoVetado=1"));
  reports(9, 60); assert(gpun_degrau() == 4);
  reports(1, 60); assert(gpun_degrau() == 3);
  reports(3, 30); assert(gpun_degrau() == 4);
  reports(10, 60); assert(gpun_degrau() == 4); // cooldown after failed recovery
  reports(90, 60); assert(gpun_degrau() < 4); // recovery is never locked forever
  reports(120, 60); assert(gpun_degrau() == 0 && gpun_fosco_vetado());
  gpun_iniciar(1920, 1080);
  assert(gpun_degrau() == 0 && gpun_fosco_vetado());
  reports(3, 30); assert(gpun_degrau() == 1);
  reports(3, 30); assert(gpun_degrau() == 3); // persisted frost step skipped
  // Forced choices are skipped in every combination, including all forced.
  for (automatic = 0; automatic < 16; automatic++) {
    gravado[0] = 0; gpun_iniciar(1920, 1080);
    for (int step = 1; step <= 4; step++) if (automatic & (1 << (step - 1))) {
      reports(3, 30); assert(gpun_degrau() == step);
    }
    int before = gpun_degrau();
    for (int i = 0; i < 20; i++) gpun_relatar(30, 40, 16.667, 0, 1);
    assert(gpun_degrau() == before);
  }
  automatic = 15;
  gravado[0] = 0; gpun_iniciar(1920, 1080);
  // Neutral FPS and stalls break streaks. Frame sampler cannot bypass ladder.
  reports(2, 30); reports(1, 56); reports(2, 30);
  assert(gpun_degrau() == 0);
  gpun_relatar(30, 1001, 16.667, 1, 1); reports(2, 30);
  assert(gpun_degrau() == 0);
  rodar(40, 40, 35, 4, 1, 1); assert(gpun_degrau() == 0);
  // Missing artwork and scene changes break streaks; healthy 50 Hz stays full.
  gravado[0] = 0; gpun_iniciar(1920, 1080);
  for (int i = 0; i < 20; i++) gpun_relatar(50, 40, 20, 1, 1);
  assert(gpun_degrau() == 0);
  reports(2, 30); gpun_relatar(30, 40, 16.667, 1, 0); reports(2, 30);
  assert(gpun_degrau() == 0);
  gpun_relatar(30, 40, 16.667, 2, 1); reports(2, 30);
  assert(gpun_degrau() == 0);
  reports(1, 30); assert(gpun_degrau() == 1);
  // GPU/driver/firmware mismatch discards the frost veto.
  reports(6, 30); assert(gpun_fosco_vetado());
  testRenderer = "Different GPU"; gpun_iniciar(1920, 1080);
  assert(!gpun_fosco_vetado());
  puts("gpunivel_escada: ok");
  return 0;
}
