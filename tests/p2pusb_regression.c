// Reuse the USB test's offline motor and network stubs.
#define main usbExistingTests
#include "p2pusb.c"
#undef main

static char mounted[800];
static int mountStat(const char *path, struct stat *st) {
  char resolved[1024];
  int rc = stat(path, st);
  size_t n = strlen(mounted);
  if (!rc && realpath(path, resolved) && n && !strncmp(resolved, mounted, n) &&
      (!resolved[n] || resolved[n] == '/')) st->st_dev++;
  return rc;
}
static unsigned scanEntries;
static int scanLstat(const char *path, struct stat *st) {
  scanEntries++;
  return lstat(path, st);
}
// Simulate a mount boundary without mounting a disk or requiring privileges.
#define lstat(path, st) scanLstat(path, st)
#define stat(path, st) mountStat(path, st)
#include "../src/p2pmotor.c"
#undef stat
#undef lstat

int main(int argc, char **argv) {
  char base[780], cache[820], det[64];
  assert(argc == 3);
  snprintf(dados, sizeof dados, "%s/dados", argv[1]);
  snprintf(base, sizeof base, "%s/regression", argv[1]);
  assert(mkdir(base, 0755) == 0);
  { char resolved[780]; assert(realpath(base, resolved)); strcpy(base, resolved); }
  snprintf(mounted, sizeof mounted, "%s/volume", base);
  snprintf(cache, sizeof cache, "%s/p2p", mounted);
  p2pmotor_teste_injetar(&OPS, NULL, NULL, NULL, 0);
  p2pmotor_teste_usb_base(base);
  p2pmotor_definir_usb(1);
  if (!strcmp(argv[2], "bounded-scan")) {
    // A rejected volume's contents must not monopolize synchronous discovery.
    assert(mkdir(mounted, 0755) == 0);
    assert(mkdir(cache, 0755) == 0); // Unowned p2p directory rejects allocation.
    for (unsigned i = 0; i < 400; i++) {
      char path[900];
      snprintf(path, sizeof path, "%s/item-%u", mounted, i);
      FILE *fp = fopen(path, "w");
      assert(fp);
      assert(fclose(fp) == 0);
    }
    usbExtra[0] = 0;
    scanEntries = 0;
    assert(!usbEscanearDir(base, cache, sizeof cache, 0, 1));
    assert(scanEntries <= 256);
  } else if (!strcmp(argv[2], "container")) {
    // A writable container is not a volume, even when empty.
    usbExtra[0] = 0;   // Exercise production mount checks, not fixture injection.
    assert(!usbEscanearDir(base, cache, sizeof cache, 0, 0));
    assert(mkdir(mounted, 0755) == 0);
    assert(usbEscanearDir(base, cache, sizeof cache, 0, 1));
    assert(existe(cache));
    snprintf(cache, sizeof cache, "%s/p2p", base);
    assert(!existe(cache));
  } else {
    // Failed allocation must expire for both UI probes and later allocation.
    // Make the container read-only so this test isolates negative caching.
    assert(chmod(base, 0555) == 0);
    assert(p2pmotor_resumo(det, sizeof det) == P2P_OK);
    assert(chmod(base, 0755) == 0);
    assert(mkdir(mounted, 0755) == 0);
    assert(chmod(base, 0555) == 0);
    usleep(2100000);
    if (!strcmp(argv[2], "retry-ui")) assert(p2pmotor_usb_presente());
    assert(p2pmotor_resumo(det, sizeof det) == P2P_OK);
    assert(existe(cache));
  }
  puts("regression ok");
  return 0;
}
