"""Execute production tile drawing branches; check focus opacity continuity."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

source = Path('src/detail.c').read_text()


def block(start, end):
    pos = source.index(start)
    return source[pos:source.index(end, pos)]


helpers = block('static float epPlacaA(', '// Largura do texto')
focus = block('  if (!epApple() && f > 0.005f) {', '  if (!epApple()) { foco_anel')
rest = block('  if (epVidroTile && f <= 0.005f) {', '  // DESFOCAR NAO ASSISTIDOS')
ceiling = next(line for line in Path('src/detail.h').read_text().splitlines()
               if line.startswith('#define NV_DETP_EP_PLACA_A '))
program = r'''
#include <stdio.h>
typedef struct { float x, y, w, h; } GfxRect;
static int glass, modern;
static float alpha;
static int draws;
static int ajustes_vidro(void) { return glass; }
static int epModerna(void) { return modern; }
static int epApple(void) { return 0; }
static float anim_clamp(float f, float lo, float hi) {
  return f < lo ? lo : f > hi ? hi : f;
}
static void gfx_cor(GfxRect p, float radius, float r, float g, float b, float a) {
  (void)p; (void)radius; (void)r; (void)g; (void)b;
  alpha = a; draws++;
}
static GfxRect epPlacaRect(GfxRect r, float h, float f) {
  (void)h; (void)f; return r;
}
''' + ceiling + '\n' + helpers + r'''
static void tile(float f, float a) {
  GfxRect r = {0, 0, 400, 400}, th = {0, 0, 400, 225};
  GfxRect epTile = {0, 225, 400, 175};
  float ft = anim_clamp(f, 0, 1);
  int epVidroTile = epModerna();
  alpha = 0; draws = 0;
''' + focus + rest + r'''
}
int main(void) {
  const float samples[] = {0, .0049f, .005f, .0051f, .006f, .01f, .1f, .5f, 1};
  for (modern = 0; modern <= 1; modern++)
    for (glass = 0; glass <= 1; glass++)
      for (int fade = 1; fade <= 2; fade++)
        for (int i = 0; i < 18; i++) {
          float f = samples[i < 9 ? i : 17-i], a = fade * .5f;
          tile(f, a);
          printf("%d %d %.6f %.6f %.9f %d\n", modern, glass, f, a, alpha, draws);
        }
}
'''
with tempfile.TemporaryDirectory(prefix='episode-tint-') as tmp:
    cfile = Path(tmp) / 'tint.c'
    binary = Path(tmp) / 'tint'
    cfile.write_text(program)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', str(cfile),
                    '-o', str(binary)], check=True)
    rows = [list(map(float, line.split())) for line in
            subprocess.check_output([str(binary)], text=True).splitlines()]

# Diagnostic strips: each column advances focus, then reverses; each row is
# glass off/on at half/full page alpha. Render measured production draw alpha.
if os.getenv('NV_EP_TINT_PNG'):
    pixels = bytearray()
    bands = [[row for row in rows if row[0] == 1 and row[1] == glass and row[3] == a]
             for glass in (0, 1) for a in (.5, 1)]
    for y in range(240):
        pixels.append(0)
        for x in range(1080):
            alpha = bands[y // 60][x // 60][4]
            value = round(24 * (1 - alpha) + 255 * alpha)
            pixels.extend((value, value, value))
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1080, 240, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(pixels)) + chunk(b'IEND', b'')
    Path(os.environ['NV_EP_TINT_PNG']).write_bytes(png)

for modern, glass, f, a, alpha, draws in rows:
    expected = .07 * (1 + f) * a if modern else (.07 if glass else .14) * f * a
    if not modern and f <= .005:
        expected = 0
    assert abs(alpha - expected) < .0004 * a, (modern, glass, f, a, alpha, expected)
    assert draws == (1 if modern or f > .005 else 0), (modern, glass, f, draws)
print('PASS: production tile tint continuous on focus entry/exit, glass off/on, page fades; Standard preserved')
