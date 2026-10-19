"""Execute the production resting tile branch across focus and glass states."""
from pathlib import Path
import subprocess
import tempfile

source = Path('src/detail.c').read_text()
start = source.index('  if (epVidroTile)\n', source.index('static void desenhaEpisodio('))
tile = source[start:source.index('  // DESFOCAR NAO ASSISTIDOS', start)]
program = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
typedef struct { float x,y,w,h; } GfxRect;
static int draws;
static float alpha;
static void gfx_cor(GfxRect r,float radius,float cr,float cg,float cb,float a) {
  (void)r; (void)radius; (void)cr; (void)cg; (void)cb;
  draws++; alpha=a;
}
static void draw(int epVidroTile,float a) {
  GfxRect epTile={0,360,640,200};
''' + tile + r'''
}
int main(void) {
  for(int modern=0;modern<=1;modern++)
    for(int focus=0;focus<=100;focus++)
      for(int fade=1;fade<=2;fade++) {
        float a=fade*.5f; draws=0; alpha=0;
        draw(modern,a);
        assert(draws==modern);
        assert(fabsf(alpha-(modern?.07f*a:0))<.00001f);
      }
  puts("PASS: resting tile opacity is independent of focus; Standard has no focus fill");
}
'''
with tempfile.TemporaryDirectory(prefix='episode-tint-') as tmp:
    cfile = Path(tmp) / 'tint.c'
    binary = Path(tmp) / 'tint'
    cfile.write_text(program)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', str(cfile),
                    '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
