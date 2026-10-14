/* Owner's stop flag vs the real transport, against a local origin that accepts
 * and never answers (no byte). See dts_engine_cancel.sh.
 * 1. rede_fluxo64 with only the owner's flag set: returns at curl's next
 *    progress tick (~1 s idle), not at the 20 s stall deadline.
 * 2. Real engine + real transport, stop given only via dts_engine_set_cancel:
 *    open and destroy (which joins the network thread) end within one tick;
 *    before the fix the flag reached curl only via the reader (~2 s). */
#include "../src/dts/dts_engine.h"
#include "../src/rede.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static volatile int own, stop;
static void *stop_later(void *u) { usleep(200000); *(volatile int *)u = 1; return NULL; }
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static int sink(void *o, const char *d, size_t n) { (void)o; (void)d; (void)n; return 1; }
static int fails;
#define CHECK(c, m) do { if (c) printf("PASS %s\n", m); else { printf("FAIL %s\n", m); fails++; } } while (0)

int main(int argc, char **argv) {
  pthread_t t;
  double t0;
  assert(argc == 2);

  { volatile int engine_flag = 0; int64_t total, got; int status, cross; char fin[8192];
    t0 = now();
    assert(!pthread_create(&t, NULL, stop_later, (void *)&own));
    int r = rede_fluxo64(argv[1], "", 0, sink, NULL, &total, &status, &engine_flag, &own, fin, &cross, 20000, &got);
    pthread_join(t, NULL);
    printf("transport: r=%d after %.3f s\n", r, now() - t0);
    CHECK(r == -1 && now() - t0 < 2.5, "rede_fluxo64 stops on the owner's flag before the first byte"); }

  { DtsEngine *e = dts_engine_create();
    dts_engine_set_cancel(e, &stop);
    t0 = now();
    assert(!pthread_create(&t, NULL, stop_later, (void *)&stop));
    int r = dts_engine_open_demux(e, argv[1], "");
    pthread_join(t, NULL);
    dts_engine_destroy(e);
    printf("engine: open=%d, destroyed after %.3f s\n", r, now() - t0);
    CHECK(r < 0 && now() - t0 < 1.5, "engine probe and teardown end promptly on the owner's flag"); }

  printf(fails ? "RESULT: %d FAIL\n" : "RESULT: all PASS\n", fails);
  return fails != 0;
}
