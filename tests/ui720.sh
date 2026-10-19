#!/bin/bash
# Offline Home/Settings PNGs, sharpness regression, alpha equality and GPU timer.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ui720.XXXXXX")
trap 'rm -rf "$work"' EXIT
out="${1:-/Volumes/ExternalSSD/tmp/rev/ui720}"
mkdir -p "$out" "$work/data"
# Existing Android SDK only; never install a compiler/runtime for this check.
angle="$HOME/Library/Android/sdk/emulator/lib64/gles_angle"
ndk=$(ls -d "$HOME"/Library/Android/sdk/ndk/*/toolchains/llvm/prebuilt/*/sysroot/usr/include 2>/dev/null | tail -1 || true)
if [ -f "$angle/libGLESv2.dylib" ] && [ -n "$ndk" ]; then
  mkdir -p "$work/inc"
  for d in GLES2 EGL KHR; do ln -s "$ndk/$d" "$work/inc/$d"; done
  cc tests/ui720_gles.c -Isrc -I"$work/inc" -L"$angle" -lEGL -lGLESv2 \
    -Wl,-rpath,"$angle" -Wl,-headerpad_max_install_names -o "$work/gles"
  for l in libEGL libGLESv2; do
    install_name_tool -change "./$l.dylib" "$angle/$l.dylib" "$work/gles"
  done
  codesign -f -s - "$work/gles" 2>/dev/null
  "$work/gles"
else
  echo "SKIP GLES2 compiler check: existing ANGLE/NDK unavailable"
fi
# Compile the actual startup surface decision with a 4K desktop and saved choices.
python3 - "$work/surface.c" <<'PY'
from pathlib import Path
import sys
main = Path('src/main.c').read_text()
block = main[main.index('{ int quer4k ='):main.index('  SDL_Window *win;', main.index('{ int quer4k ='))]
Path(sys.argv[1]).write_text('''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define NV_TELA_W 1920
#define NV_TELA_H 1080
#define NV_VERSAO "test"
#define RES_ARQ_RECUO "fallback"
#define RES_ARQ_AUTO "automatic"
#define RES_AUTO_1080 2
#define RES_AUTO_4K 1
typedef struct { int w; } SDL_DisplayMode;
static int choice;
static int resolucaoDaTv(void) { return 1; }
static int ajustes_4k(void) { return choice==2; }
static int ajustes_1080p(void) { return choice==1; }
static int ajustes_720p(void) { return choice==3; }
static int ajustes_res_auto(void) { return choice==0; }
static int SDL_GetDesktopDisplayMode(int d, SDL_DisplayMode *m) { (void)d; m->w=3840; return 0; }
static char *dados_ler(const char *p) { (void)p; return NULL; }
static int res_auto_ler(const char *p,const char *v) { (void)p;(void)v;return 0; }
int main(void) {
  for(choice=1;choice<=3;choice++) {
    int pedeW=0,pedeH=0,pediu4k=0,autoEst=0,autoPediu=0;
''' + block + '''
    if (choice==1 && (pedeW!=1920 || pedeH!=1080)) {
      fprintf(stderr,"FAIL explicit 1080p requested %dx%d\\n",pedeW,pedeH);
      return 1;
    }
    assert(choice==1 || (pedeW==3840 && pedeH==2160));
  }
  puts("PASS explicit 1080p keeps 1920x1080 surface on 4K desktop");
}
''')
PY
cc "$work/surface.c" -o "$work/surface"
surface_rc=0
"$work/surface" || surface_rc=$?
printf 'animacoes 0\nidioma 1\nhomeLayoutLocal 0\n' > "$work/data/ajustes.txt"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/gfx.c|src/gpunivel.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ui720.c -Isrc -o "$work/test" -O1 \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations -Wno-macro-redefined
ui_rc=0
NUVIO_DADOS="$work/data" "$work/test" "$out" > "$out/results.txt" 2>&1 || ui_rc=$?
rg '^(GPU |METRIC |BLIT |FRAME |PASS |FAIL |\[gpu-nivel\] upscale=)' "$out/results.txt"
[ "$surface_rc" -eq 0 ] && [ "$ui_rc" -eq 0 ]
