"""Compile production startup choices against the integration baseline, without SDL."""
from pathlib import Path
import subprocess
import tempfile

main = Path('src/main.c').read_text()
base = subprocess.check_output(['git', 'show', 'integracao/2.0.5:src/main.c'], text=True)

def decision(source):
    start = source.index('{ int quer4k =')
    end = source.index('  const char *windowTitle', start)
    return source[start:end].replace('  SDL_Window *win;', '')

prefix = r'''
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
typedef struct { int w, h; } SDL_DisplayMode;
static int choice, app, desktop, panel, tv=1, surfaceW, surfaceH, fallback;
static int resolucaoDaTv(void) { return tv; }
static int ajustes_4k(void) { return choice==2; }
static int ajustes_1080p(void) { return choice==1; }
static int ajustes_720p(void) { return choice==3; }
static int ajustes_res_auto(void) { return choice==0; }
static int ajustes_ampliar_app(void) { return app && panel>=3840; }
static int SDL_GetDesktopDisplayMode(int d, SDL_DisplayMode *m) {
  (void)d; m->w=desktop; m->h=desktop*9/16; return 0;
}
static int ajustes_painel4k(void) { return panel>=3840; }
static char *dados_ler(const char *p) {
  if (fallback && p[0]=='f') { char *m=malloc(2); m[0]='1'; m[1]=0; return m; }
  return NULL;
}
static int res_auto_ler(const char *p,const char *v) { (void)p;(void)v;return 0; }
static int android_pedir_superficie(int w,int h) { surfaceW=w;surfaceH=h;return 1; }
static void tpk_pedir_saida(int w,int h) { (void)w;(void)h; }
'''
body = r'''
int main(void) {
 for(int d=0;d<3;d++) for(int p=0;p<2;p++) for(tv=0;tv<2;tv++) for(choice=0;choice<4;choice++) {
  desktop=d==0?1280:d==1?1920:3840;
  panel=p?3840:1920;
  int oldW,oldH,oldSW,oldSH;
  {int pedeW=0,pedeH=0,pediu4k=0,autoEst=0,autoPediu=0,recuo1080=0;(void)recuo1080;
   surfaceW=surfaceH=0; app=0;
BASE
   oldW=pedeW;oldH=pedeH;oldSW=surfaceW;oldSH=surfaceH;
  }
  for(app=0;app<2;app++) {
   int pedeW=0,pedeH=0,pediu4k=0,autoEst=0,autoPediu=0,recuo1080=0;
   surfaceW=surfaceH=0;
CURRENT
#ifdef NV_ANDROID
   if(choice==0 && tv && panel==3840 && desktop<3840) {
    // Forced display size: Automatic still sees the 4K panel and probes it.
    assert(pedeW==3840 && pedeH==2160 && surfaceW==3840);
    continue;
   }
#endif
   if(app && panel==3840 && tv && (choice==1 || choice==3)) {
    assert(pedeW==3840 && pedeH==2160);
#ifdef NV_ANDROID
    assert(surfaceW==3840 && surfaceH==2160);
#endif
   } else if(pedeW!=oldW || pedeH!=oldH || surfaceW!=oldSW || surfaceH!=oldSH) {
    fprintf(stderr,"FAIL baseline: desktop=%d TV=%d resolution=%d app=%d: %dx%d vs %dx%d; Android %dx%d vs %dx%d\n",
      desktop,tv,choice,app,pedeW,pedeH,oldW,oldH,surfaceW,surfaceH,oldSW,oldSH);
    return 1;
   }
  }
 }
 // 4K that did not hold, with app scaling: 1080p target on the 4K surface.
 for(app=0;app<2;app++) for(panel=1920;panel<=3840;panel+=1920) {
  int pedeW=0,pedeH=0,pediu4k=0,autoEst=0,autoPediu=0,recuo1080=0;
  tv=1; choice=2; desktop=3840; fallback=1; surfaceW=surfaceH=0;
CURRENT
  fallback=0;
  int want=app && panel==3840;
  assert(recuo1080==want && pediu4k==0);
  assert(pedeW==(want?3840:1920));
 }
 puts("PASS startup baseline and opt-in 4K: all resolutions, panel sizes, TV/desktop");
}
'''.replace('BASE', decision(base)).replace('CURRENT', decision(main))
with tempfile.TemporaryDirectory(prefix='ui720-surface-') as work:
    source = Path(work) / 'surface.c'
    source.write_text(prefix + body)
    for platform in ('NV_WEBOS', 'NV_TPK', 'NV_ANDROID'):
        binary = Path(work) / platform
        subprocess.run(['cc', '-D' + platform, str(source), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, stdout=subprocess.DEVNULL)
        print('PASS', platform, 'integration drawable parity; physical 4K + 720/1080 desktop requests 2160')
