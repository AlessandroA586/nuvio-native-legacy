// Compile the production C ROI/window path with controllable AVPlay results.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const source = fs.readFileSync('src/video_tizen.c', 'utf8');
const start = source.indexOf('static int aplicarRect(');
const end = source.indexOf('\n// RECORTE DE FONTE:', start);
assert(start >= 0 && end > start, 'production ROI/window path not found');
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'avroi-path-'));
try {
  const fixture = `
#define main helper_main
#include "${path.resolve('tests/avroi.c')}"
#undef main
#include <string.h>
static int roiAtivo, temAvplay = 1, ativo = 1;
static int janX, janY, janW, janH;
static int roiCalls, roi[4];
static double result = 1;
static double video_largura(void) { return 1920; }
static double video_altura(void) { return 1080; }
static double avChamar(const char *op, const char *txt,
                      double a, double b, double c, double d, char *dst, int size) {
  (void)txt; (void)dst; (void)size;
  if (strcmp(op, "roi")) return 1;
  roiCalls++;
  roi[0] = a; roi[1] = b; roi[2] = c; roi[3] = d;
  return result;
}
${source.slice(start, end)}
int main(void) {
  bad = helper_main();
  EXPECT(roiAplicar(100, 0, 1600, 1080) == 1);
  EXPECT(roiAtivo == 1 && roiCalls == 1 && roi[0] == 100 && roi[2] == 1600);
  result = 0; // transient rejection while leaving zoom for PiP
  video_janela(20, 20, 640, 360);
  EXPECT(roiCalls == 2 && roiAtivo == 1);
  EXPECT(roi[0] == 0 && roi[1] == 0 && roi[2] == 1920 && roi[3] == 1080);
  result = 1;
  video_janela(20, 20, 640, 360); // unchanged destination must still retry ROI
  EXPECT(roiCalls == 3 && roiAtivo == 0);
  video_janela(0, 0, 1920, 1080);
  EXPECT(roiCalls == 3);
  EXPECT(roiAplicar(100, 0, 1600, 1080) == 1);
  result = -1; // missing API is distinct from transient rejection
  video_janela(0, 0, 1920, 1080);
  EXPECT(roiAtivo == 0 && roiCalls == 5);
  video_janela(0, 0, 1920, 1080);
  EXPECT(roiCalls == 5);
  if (!bad) puts("PASS production ROI reset/retry");
  return bad;
}
`;
  const file = path.join(dir, 'test.c');
  const binary = path.join(dir, 'test');
  fs.writeFileSync(file, fixture);
  for (const [cmd, args] of [
    ['cc', ['-Wall', '-Wextra', '-Werror', file, '-o', binary]],
    [binary, []],
  ]) {
    const run = spawnSync(cmd, args, { stdio: 'inherit' });
    if (run.error) throw run.error;
    if (run.status !== 0) { process.exitCode = run.status || 1; break; }
  }
} finally {
  fs.rmSync(dir, { recursive: true, force: true });
}
