#define main subtitle_fixture_main
#include "addons_legendas.c"
#undef main

static void *replace(void *unused) {
  (void)unused;
  pthread_mutex_lock(&legTrava);
  snprintf(legs[0].url, sizeof legs[0].url, "new subtitle");
  pthread_mutex_unlock(&legTrava);
  const Legenda *other = addons_legenda(0);
  assert(other && !strcmp(other->url, "new subtitle"));
  return NULL;
}

int main(void) {
  pthread_t worker;
  nLegs = 1;
  snprintf(legs[0].url, sizeof legs[0].url, "original subtitle");
  const Legenda *copy = addons_legenda(0);
  assert(copy);
  assert(!pthread_create(&worker, NULL, replace, NULL));
  pthread_join(worker, NULL);
  assert(!strcmp(copy->url, "original subtitle"));
  assert(!addons_legenda(-1) && !addons_legenda(1));
  assert(!strcmp(addons_legenda(0)->url, "new subtitle"));
  puts("PASS: subtitle copy survives worker replacement and another thread's getter");
  return 0;
}
