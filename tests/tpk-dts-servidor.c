// Host test of the .tpk DTS HLS server (src/dts/dts_hls_tpk.c, real) against a
// fake converter: stop cancels a probe that is still blocked on the network,
// listener is loopback-only and closed on stop, connection threads are capped,
// Range handling (inverted -> 416, HEAD from the cache sends headers only).
#include "dts_hls_tpk.h"
#include "dts_remux.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int fails;
#define CHECK(c, m) do { if (c) printf("PASS %s\n", m); else { printf("FAIL %s\n", m); fails++; } } while (0)

// ---- fake converter ------------------------------------------------------------
struct DtsRemux { DtsRemuxInfo in; volatile int *cancel; };
struct DtsRemuxGen { int x; };
static volatile int *g_probe_cancel;   // flag the server handed to dts_remux_open
static int g_probe_seen;

int dts_remux_available(void) { return 1; }
void dts_engine_set_ring_mib(int mib) { (void)mib; }
void dts_remux_set_queue_budget_mib(int mib) { (void)mib; }
DtsRemux *dts_remux_open(const char *url, const char *h, int ord, int st, double seg, volatile int *cancel,
                         char *err, size_t errn) {
  (void)h; (void)ord; (void)st; (void)seg;
  if (strstr(url, "hang")) {           // blocked on the network until the session is cancelled
    g_probe_cancel = cancel; g_probe_seen = 1;
    while (!*cancel) usleep(5000);
    snprintf(err, errn, "cancelled");
    return NULL;
  }
  DtsRemux *r = calloc(1, sizeof *r);
  r->in.duration_s = 30; r->in.n_segments = 3; r->in.index_complete = 1; r->in.max_segment_s = 6;
  r->in.dv_profile = -1; r->in.size_bytes = 1000; r->cancel = cancel;
  snprintf(err, errn, "%s", "");
  return r;
}
const DtsRemuxInfo *dts_remux_info(const DtsRemux *r) { return &r->in; }
double dts_remux_segment_start(const DtsRemux *r, int s) { (void)r; return s * 6.0; }
double dts_remux_segment_duration(const DtsRemux *r, int s) { (void)r; (void)s; return 6.0; }
int dts_remux_segment_at(const DtsRemux *r, double t) { (void)r; return (int)(t / 6); }
double dts_remux_keyframe_before(const DtsRemux *r, double t) { (void)r; return t; }
void dts_remux_resegment(DtsRemux *r, double t) { (void)r; (void)t; }
void dts_remux_cancel(DtsRemux *r) { (void)r; }
void dts_remux_close(DtsRemux *r) { free(r); }
DtsRemuxGen *dts_remux_gen_new(DtsRemux *r, char *e, size_t n) { (void)r; (void)e; (void)n; return calloc(1, sizeof(DtsRemuxGen)); }
int dts_remux_gen_next(const DtsRemuxGen *g) { (void)g; return -1; }
int dts_remux_gen_run(DtsRemuxGen *g, int seg, double t, const char *fmt, DtsRemuxWrite w, void *o, volatile int *stop,
                      DtsRemuxStats *st, char *e, size_t n) {
  unsigned char b[1000];
  (void)g; (void)seg; (void)t; (void)fmt; (void)stop; (void)e; (void)n;
  memset(b, 0x47, sizeof b);
  st->fresh = 1;
  return w(o, b, sizeof b) < 0 ? -2 : 0;
}
void dts_remux_gen_free(DtsRemuxGen *g) { free(g); }
void dts_remux_gen_interrupt(DtsRemuxGen *g) { (void)g; }
double dts_remux_gen_start_s(const DtsRemuxGen *g) { (void)g; return 0; }

// ---- helpers --------------------------------------------------------------------
static double now(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}
static int porta_de(const char *url) { const char *p = strstr(url, "127.0.0.1:"); return p ? atoi(p + 10) : 0; }
static int conectar(int porta) {
  struct sockaddr_in a;
  struct timeval tv = { 5, 0 };
  int s = socket(AF_INET, SOCK_STREAM, 0);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_port = htons((unsigned short)porta); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
  return s;
}
// One request on a fresh connection; the whole response (until close or timeout) in out.
static int pedir(int porta, const char *req, char *out, size_t n) {
  int s = conectar(porta), tot = 0;
  out[0] = 0;
  if (s < 0) return -1;
  send(s, req, strlen(req), 0);
  for (;;) {
    ssize_t r = recv(s, out + tot, n - 1 - (size_t)tot, 0);
    if (r <= 0) break;
    tot += (int)r; out[tot] = 0;
    // Content-Length responses: stop once the header says all arrived.
    char *he = strstr(out, "\r\n\r\n");
    if (he) {
      char *cl = strstr(out, "Content-Length: ");
      long len = cl ? atol(cl + 16) : 0;
      int head = !strncmp(req, "HEAD", 4);
      if (head || tot >= (int)(he + 4 - out) + len) break;
    }
  }
  close(s);
  return tot;
}
static void esperar_pronta(void) {
  double t0 = now();
  while (dts_tpk_estado() == DTSTPK_SONDANDO && now() - t0 < 5) usleep(5000);
}

int main(void) {
  char url[256], resp[8192], req[512], tok[16];
  int porta, n;

  // --- 3b: stop while the probe is blocked on the network ---------------------
  CHECK(dts_tpk_abrir("http://x/hang.mkv", "", -1, 0, DTSTPK_HLS, DTSTPK_CORPO_LENGTH, 0, url, sizeof url) == 1, "open session on a hanging source");
  { double t0 = now();
    while (!g_probe_seen && now() - t0 < 2) usleep(2000); }
  CHECK(g_probe_seen && g_probe_cancel != NULL, "probe is blocked in the converter");
  { double t0 = now();
    dts_tpk_fechar();
    CHECK(now() - t0 < 1.0, "stop returns without waiting for the probe");
    CHECK(g_probe_cancel && *g_probe_cancel == 1, "stop sets the cancel flag handed to the probe"); }

  // --- 3d: loopback only, and closed after stop --------------------------------
  CHECK(dts_tpk_abrir("http://x/ok.mkv", "", -1, 0, DTSTPK_HLS, DTSTPK_CORPO_LENGTH, 0, url, sizeof url) == 1, "open session");
  porta = porta_de(url);
  { const char *t = strstr(url, "127.0.0.1:"); t = t ? strchr(t, '/') : NULL;
    snprintf(tok, sizeof tok, "%.8s", t ? t + 1 : ""); }
  CHECK(porta > 0, "url names 127.0.0.1 and a port");
  esperar_pronta();
  CHECK(dts_tpk_estado() == DTSTPK_PRONTO, "probe ready");

  // --- 3e: Range ----------------------------------------------------------------
  snprintf(req, sizeof req, "GET /%s/seg0.ts HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n", tok);
  n = pedir(porta, req, resp, sizeof resp);
  CHECK(n > 0 && strstr(resp, " 200 ") != NULL && strstr(resp, "Content-Length: 1000") != NULL, "segment 0 generated (200, 1000 B)");
  snprintf(req, sizeof req, "GET /%s/seg0.ts HTTP/1.1\r\nHost: x\r\nRange: bytes=8-0\r\nConnection: close\r\n\r\n", tok);
  n = pedir(porta, req, resp, sizeof resp);
  CHECK(n > 0 && strstr(resp, " 416 ") != NULL, "inverted Range bytes=8-0 -> 416");
  snprintf(req, sizeof req, "GET /%s/seg0.ts HTTP/1.1\r\nHost: x\r\nRange: bytes=10-19\r\nConnection: close\r\n\r\n", tok);
  n = pedir(porta, req, resp, sizeof resp);
  CHECK(n > 0 && strstr(resp, " 206 ") != NULL && strstr(resp, "Content-Range: bytes 10-19/1000") != NULL, "valid Range -> 206");
  snprintf(req, sizeof req, "HEAD /%s/seg0.ts HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n", tok);
  n = pedir(porta, req, resp, sizeof resp);
  CHECK(n > 0 && strstr(resp, " 200 ") != NULL && strstr(resp, "Content-Length: 1000") != NULL, "HEAD on the cached segment sends headers");
  CHECK(n > 0 && strstr(resp, "\r\n\r\n") && *(strstr(resp, "\r\n\r\n") + 4) == 0, "HEAD sends no body");

  // --- 3d: connection cap ---------------------------------------------------------
  { int idle[64], k, ok = 0;
    for (k = 0; k < 64; k++) idle[k] = conectar(porta);
    usleep(300 * 1000);
    snprintf(req, sizeof req, "GET /%s/v.m3u8 HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n", tok);
    double t0 = now();
    n = pedir(porta, req, resp, sizeof resp);
    ok = n > 0 && strstr(resp, " 503 ") != NULL && now() - t0 < 2.0;
    CHECK(ok, "over the connection cap: 503 at once (no new thread)");
    for (k = 0; k < 64; k++) if (idle[k] >= 0) close(idle[k]);
    usleep(500 * 1000);
    n = pedir(porta, req, resp, sizeof resp);
    CHECK(n > 0 && strstr(resp, " 200 ") != NULL, "after the idle connections close the server answers again"); }

  // --- stop drops the accepted connections: their slots are free at once -----------
  { int idle[16], k, eof = 0;
    char c;
    for (k = 0; k < 16; k++) idle[k] = conectar(porta);
    usleep(300 * 1000);                    // all 16 accepted, parked in the 15 s request wait
    dts_tpk_fechar();
    CHECK(dts_tpk_abrir("http://x/ok.mkv", "", -1, 0, DTSTPK_HLS, DTSTPK_CORPO_LENGTH, 0, url, sizeof url) == 1, "reopen right after stop");
    porta = porta_de(url);
    { const char *t = strstr(url, "127.0.0.1:"); t = t ? strchr(t, '/') : NULL;
      snprintf(tok, sizeof tok, "%.8s", t ? t + 1 : ""); }
    esperar_pronta();
    usleep(100 * 1000);
    snprintf(req, sizeof req, "GET /%s/v.m3u8 HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n", tok);
    n = pedir(porta, req, resp, sizeof resp);
    CHECK(n > 0 && strstr(resp, " 200 ") != NULL, "new player gets 200 while the old clients are still open");
    for (k = 0; k < 16; k++) if (idle[k] >= 0 && recv(idle[k], &c, 1, MSG_DONTWAIT) == 0) eof++;
    CHECK(eof == 16, "stop closed the old accepted connections (EOF)");
    for (k = 0; k < 16; k++) if (idle[k] >= 0) close(idle[k]); }

  // --- 3d: listener closed on stop ---------------------------------------------------
  dts_tpk_fechar();
  usleep(600 * 1000);
  { int s = conectar(porta);
    CHECK(s < 0, "listener closed after stop (connect refused)");
    if (s >= 0) close(s); }
  CHECK(dts_tpk_e_url(url) == 0, "stop forgets the server address");

  printf(fails ? "RESULT: %d FAIL\n" : "RESULT: all PASS\n", fails);
  return fails != 0;
}
