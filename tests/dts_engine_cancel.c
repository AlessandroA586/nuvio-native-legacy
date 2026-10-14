/* Owner's stop flag vs the real transport, against a local origin that accepts
 * and never answers (no byte). See dts_engine_cancel.sh.
 * 1. rede_fluxo64 with only the owner's flag set: returns at curl's next
 *    progress tick (~1 s idle), not at the 20 s stall deadline.
 * 2. Real engine + real transport, stop given only via dts_engine_set_cancel:
 *    open and destroy (which joins the network thread) end within one tick;
 *    before the fix the flag reached curl only via the reader (~2 s).
 * The fake build blocks before any sink/callback and requires cancellation
 * within 600 ms, so a periodic reader check cannot hide a missing flag. */
#include "../src/dts/dts_engine.h"
#include "../src/rede.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static volatile int own, stop;
static void *stop_later(void *u) { usleep(200000); __atomic_store_n((volatile int *)u, 1, __ATOMIC_RELAXED); return NULL; }
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static int sink(void *o, const char *d, size_t n) { (void)o; (void)d; (void)n; return 1; }
#ifdef DTS_FAKE_TRANSPORT
/* No body/sink call and no engine callback: only the flags handed to I/O can
 * stop this stand-in for DNS/connect. Three seconds bounds a broken test. */
char *rede_baixar_trecho64_cab(const char *u,const char *h,int64_t a,int64_t b,long *n,int64_t *t,int *s,volatile int *c) {
  (void)u;(void)h;(void)a;(void)b;(void)n;(void)t;(void)s;(void)c; return NULL;
}
char *rede_baixar_trecho64_final(const char *u,const char *h,int64_t a,int64_t b,long *n,int64_t *t,int *s,volatile int *c,char *f,int *x) {
  (void)f;(void)x; return rede_baixar_trecho64_cab(u,h,a,b,n,t,s,c);
}
int rede_fluxo64(const char *u,const char *h,int64_t a,int (*w)(void *,const char *,size_t),void *o,
                int64_t *t,int *s,volatile int *c,volatile int *c2,char *f,int *x,long ms,int64_t *b) {
  (void)u;(void)h;(void)a;(void)w;(void)o;(void)t;(void)s;(void)f;(void)x;(void)ms;(void)b;
  double deadline = now() + 3;
  while (!(c && __atomic_load_n(c, __ATOMIC_RELAXED)) &&
         !(c2 && __atomic_load_n(c2, __ATOMIC_RELAXED)) && now() < deadline) usleep(1000);
  return -1;
}
#define ENGINE_DEADLINE 0.6
#else
#define ENGINE_DEADLINE 1.5
#endif
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
    CHECK(r < 0 && now() - t0 < ENGINE_DEADLINE, "engine probe and teardown end promptly on the owner's flag"); }

  printf(fails ? "RESULT: %d FAIL\n" : "RESULT: all PASS\n", fails);
  return fails != 0;
}
