// Offline SGSR proof: real UI, surface-size references, alpha, GPU/host medians.
#include "gfx.c" // Reuse the real shader compiler; round 1 exists only in this benchmark.
#include "app.h"
#include "ajustes.h"
#include "addons.h"
#include "fileiras.h"
#include "artehero.h"
#include "catalogo.h"
#include "dados.h"
#include "gpunivel.h"
#include "home.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Exercise weak-GPU dispatch without replacing the real drawing context.
static int pretendUtgard;
static const GLubyte *testGlString(GLenum e) {
  if (pretendUtgard && e==GL_RENDERER) return (const GLubyte*)"Mali-450";
  return glGetString(e);
}
#define glGetString testGlString
#define telaW gpunTelaW
#define telaH gpunTelaH
#include "../src/gpunivel.c"
#undef telaW
#undef telaH
#undef glGetString

static GLuint source, sourceFbo, query;
static int sw=1920, sh=1080, iw=1280, ih=720;
static unsigned char *pixels, *flipped, *linear, *native;
static const GfxRect screen = {0,0,1920,1080};
static const char *round1 =
  "void main(){ vec2 uv=vec2(vUv.x,1.0-vUv.y), d=1.0/uPar;"
  "vec4 c=texture2D(uTex,uv);"
  "vec3 n=texture2D(uTex,uv+vec2(d.x,0.0)).rgb+texture2D(uTex,uv-vec2(d.x,0.0)).rgb"
  "+texture2D(uTex,uv+vec2(0.0,d.y)).rgb+texture2D(uTex,uv-vec2(0.0,d.y)).rgb;"
  "c.rgb=clamp(c.rgb*1.8-n*0.2,0.0,1.0); gl_FragColor=c;}";
static void readback(void) {
  glReadPixels(0,0,sw,sh,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  assert(glGetError()==GL_NO_ERROR);
}
static void png(const char *dir, const char *name) {
  char path[1024];
  readback();
  for (int y=0;y<sh;y++) memcpy(flipped+(size_t)y*sw*4,pixels+(size_t)(sh-1-y)*sw*4,sw*4);
  SDL_Surface *s=SDL_CreateRGBSurfaceWithFormatFrom(flipped,sw,sh,32,sw*4,SDL_PIXELFORMAT_RGBA32);
  assert(s); snprintf(path,sizeof path,"%s/%s.png",dir,name);
  assert(IMG_SavePNG(s,path)==0); SDL_FreeSurface(s);
}
static void frame(int settings) {
  txt_novo_quadro(); tex_novo_quadro(); tex_bombear(16); gfx_novo_quadro();
  gfx_sem_recorte();
  glClearColor(0.051f,0.051f,0.051f,1); glClear(GL_COLOR_BUFFER_BIT);
  if (settings) ajustes_desenhar(10000); else home_desenhar(10000);
  gfx_ambiente_descarregar();
  gfx_furo((GfxRect){1500,200,180,120});
  gfx_furo_raio((GfxRect){1500,400,180,120},24);
  gfx_furo_alfa((GfxRect){1500,600,180,120},0.45f);
}
static void target(int low) {
  glBindFramebuffer(GL_FRAMEBUFFER,low?sourceFbo:0);
  glViewport(0,0,low?iw:sw,low?ih:sh); gfx_tamanho_alvo(low?iw:sw,low?ih:sh);
}
static void copy(int mode) {
  target(0); gfx_sem_recorte(); glDisable(GL_BLEND);
  gfx_tex_aspect_atual=0; gfx_opacidade_grupo=1;
  gfx_rect(screen,source,GFX_COPIA,0,mode?iw:0,mode?ih:1,0,1,1,1,1);
  glEnable(GL_BLEND);
}
static double gradient(const unsigned char *p, int settings) {
  int x0=(settings?180:100)*sw/1920, x1=(settings?600:700)*sw/1920;
  int y0=(settings?180:620)*sh/1080, y1=(settings?950:845)*sh/1080;
  double sum=0;
  for (int y=y0;y<y1;y++) for (int x=x0;x<x1;x++) {
    size_t i=((size_t)(sh-1-y)*sw+x)*4;
    for (int c=0;c<3;c++) sum+=abs(p[i+c]-p[i+4+c])+abs(p[i+c]-p[i+sw*4+c]);
  }
  return sum/((x1-x0)*(y1-y0)*6.0);
}
static double error(const unsigned char *p, int settings) {
  int x0=(settings?180:100)*sw/1920, x1=(settings?600:700)*sw/1920;
  int y0=(settings?180:620)*sh/1080, y1=(settings?950:845)*sh/1080;
  double sum=0;
  for (int y=y0;y<y1;y++) for (int x=x0;x<x1;x++) {
    size_t i=((size_t)(sh-1-y)*sw+x)*4;
    for (int c=0;c<3;c++) sum+=abs(p[i+c]-native[i+c]);
  }
  return sum/((x1-x0)*(y1-y0)*3.0);
}
static double median(double *s) {
  for(int i=0;i<15;i++) for(int j=i+1;j<15;j++) if(s[j]<s[i]) {double t=s[i];s[i]=s[j];s[j]=t;}
  return s[7];
}
static void timing(int settings, int mode, int whole, double *host, double *gpu) {
  double hs[15],gs[15];
  for(int k=-4;k<15;k++) {
    glFinish(); Uint64 start=SDL_GetPerformanceCounter();
    glBeginQuery(GL_TIME_ELAPSED_EXT,query);
    if(whole) {target(mode!=0); frame(settings);}
    if(!whole || mode) copy(mode==1?0:1);
    glEndQuery(GL_TIME_ELAPSED_EXT); glFinish();
    double ms=1000.0*(SDL_GetPerformanceCounter()-start)/SDL_GetPerformanceFrequency();
    GLuint64 ns; glGetQueryObjectui64vEXT(query,GL_QUERY_RESULT,&ns);
    if(k>=0) {hs[k]=ms;gs[k]=ns/1e6;}
  }
  *host=median(hs); *gpu=median(gs);
}
int main(int argc,char **argv) {
  assert(argc==2); SDL_SetHint("SDL_MAC_BACKGROUND_APP","1");
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
  SDL_Window *window=SDL_CreateWindow("SGSR proof",0,0,3840,2160,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
  assert(window && SDL_GL_CreateContext(window)); SDL_GL_SetSwapInterval(0);
  assert(gfx_iniciar() && txt_iniciar("deploy/app",2));
  dados_iniciar("deploy/app/art"); ajustes_dir(getenv("NUVIO_DADOS"));
  tex_iniciar(64); gfx_icones_dir("deploy/app/art"); artehero_definir_falhou(tex_falhou);
  assert(home_iniciar("deploy/app/art"));
  AddonRemoto addon={.nome="Offline",.url="local",.ativo=1}; assert(addons_definir_lista(&addon,1));
  fil_registrar("offline_movie","Popular movies - Offline catalogue","Offline","Filmes",12);
  assert(fil_definir_tipo("offline_movie",FIL_TIPO_CARTAZ));
  static CatItem items[12]; CatFileira row={.ini=0,.n=12};
  snprintf(row.chave,sizeof row.chave,"offline_movie");
  snprintf(row.titulo,sizeof row.titulo,"Popular movies - Offline catalogue");
  snprintf(row.tipo,sizeof row.tipo,"movie");
  for(int i=0;i<12;i++) {
    snprintf(items[i].imdb,sizeof items[i].imdb,"tt90%05d",i);
    snprintf(items[i].tipo,sizeof items[i].tipo,"movie");
    snprintf(items[i].titulo,sizeof items[i].titulo,"The sharpness of small text %d",i+1);
    snprintf(items[i].sinopse,sizeof items[i].sinopse,
      "A local story about distant worlds. Every detail matters when reading from the sofa. "
      "Explore the catalogue, choose a movie and continue watching.");
    snprintf(items[i].logo,sizeof items[i].logo,"deploy/app/art/icone-home.png");
    snprintf(items[i].meta,sizeof items[i].meta,"2026 · 120 min");
    snprintf(items[i].poster,sizeof items[i].poster,"deploy/app/art/poster/%02d.jpg",i%8);
    snprintf(items[i].backdrop,sizeof items[i].backdrop,"deploy/app/art/%02d.jpg",i%8);
  }
  cat_definir_tudo(items,12,&row,1);
  target(0);
  for(int n=0;n<120;n++) {home_atualizar(1.f/60,10000);frame(0);}
  assert(ajustes_iniciar());
  assert(strstr((const char*)glGetString(GL_EXTENSIONS),"GL_EXT_timer_query"));
  printf("GPU %s; GL_EXT_timer_query; synchronized host wall-time medians, 15 frames\n",glGetString(GL_RENDERER));
  glGenQueries(1,&query);
  // Keep the obsolete round-1 shader in tests only, using the same production quad.
  __typeof__(progs[0]) sgsr=progs[GFX_COPIA];
  FS_CORPO[GFX_COPIA]=round1; assert(progMontar(GFX_COPIA));
  __typeof__(progs[0]) old=progs[GFX_COPIA];
  progs[GFX_COPIA]=sgsr; progAtual=-1;
  size_t capacity=(size_t)3840*2160*4;
  pixels=malloc(capacity);flipped=malloc(capacity);linear=malloc(capacity);native=malloc(capacity);
  assert(pixels&&flipped&&linear&&native);
  int failed=0;
  for(int test=0;test<3;test++) {
    sw=test?3840:1920;sh=test?2160:1080;iw=test==2?1920:1280;ih=test==2?1080:720;
    size_t bytes=(size_t)sw*sh*4;
    glGenTextures(1,&source);glBindTexture(GL_TEXTURE_2D,source);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,iw,ih,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);gfx_tex_esquecer(0);
    glGenFramebuffers(1,&sourceFbo);glBindFramebuffer(GL_FRAMEBUFFER,sourceFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,source,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    gpun_iniciar(sw,sh);gpun_definir_nivel(0);
    for(int settings=0;settings<2;settings++) for(int glass=0;glass<2;glass++) {
      const char *scene=settings?"settings":"home"; char name[160];
      ajustes_definir_vidro(glass);
      target(0);for(int n=0;n<8;n++) frame(settings);
      readback();memcpy(native,pixels,bytes);double ref=gradient(native,settings);
      if(!glass) {snprintf(name,sizeof name,"native-%d-%s",sh,scene);png(argv[1],name);}
      target(1);for(int n=0;n<8;n++) frame(settings);
      copy(0);readback();memcpy(linear,pixels,bytes);
      double b=gradient(linear,settings), be=error(linear,settings);
      if(!glass) {snprintf(name,sizeof name,"bilinear-%d-%d-%s",ih,sh,scene);png(argv[1],name);}
      progs[GFX_COPIA]=old;progAtual=-1;copy(1);readback();
      double r=gradient(pixels,settings), re=error(pixels,settings), rh,rg;
      timing(settings,2,0,&rh,&rg);
      progs[GFX_COPIA]=sgsr;progAtual=-1;copy(1);readback();
      double s=gradient(pixels,settings), se=error(pixels,settings);
      for(size_t i=3;i<bytes;i+=4) assert(pixels[i]==linear[i]);
      assert(pixels[((size_t)(sh-1-250*sh/1080)*sw+1550*sw/1920)*4+3]==0);
      if(!glass) {
        snprintf(name,sizeof name,"sgsr-%d-%d-%s",ih,sh,scene);png(argv[1],name);
        if(!test) {snprintf(name,sizeof name,"sgsr-%s",scene);png(argv[1],name);}
      }
      // Recover at least 5% of lost native text contrast and beat both RGB errors.
      double threshold=b+0.05*fmax(0.0,ref-b);
      int pass=s>threshold && fabs(s-ref)<fabs(b-ref) && se<be && se<re;
      printf("METRIC %d->%d %s glass=%d gradient bilinear=%.6f round1=%.6f SGSR=%.6f native=%.6f; MAE %.4f/%.4f/%.4f; threshold=%.6f before=%s after=%s; alpha identical\n",
        ih,sh,scene,glass,b,r,s,ref,be,re,se,threshold,b>threshold?"PASS":"FAIL",pass?"PASS":"FAIL");
      if(!pass) failed=1;
      double bh,bg,ss,sg;timing(settings,1,0,&bh,&bg);timing(settings,2,0,&ss,&sg);
      printf("BLIT %d->%d %s glass=%d host/GPU ms bilinear=%.4f/%.4f round1=%.4f/%.4f SGSR=%.4f/%.4f\n",ih,sh,scene,glass,bh,bg,rh,rg,ss,sg);
      double nh,ng,lh,lg;timing(settings,0,1,&nh,&ng);timing(settings,1,1,&lh,&lg);timing(settings,2,1,&ss,&sg);
      printf("FRAME %d->%d %s glass=%d level=0 host/GPU ms native=%.4f/%.4f bilinear=%.4f/%.4f SGSR=%.4f/%.4f\n",ih,sh,scene,glass,nh,ng,lh,lg,ss,sg);
    }
    // Real gpun_quadro_fim must match the benchmark, including 1080p->4K at level 0.
    ajustes_definir_vidro(0);target(1);frame(0);copy(1);readback();memcpy(linear,pixels,bytes);
    if(test==2) gpun_alvo_1080();else gpun_definir_nivel(3);
    // Level 0 effects, even when today's 720p setting selects level 3.
    gfx_definir_efeitos_leves(0);gfx_definir_efeitos_minimos(0);
    gpun_quadro_inicio();frame(0);gpun_quadro_fim();readback();assert(!memcmp(linear,pixels,bytes));
    glDeleteFramebuffers(1,&sourceFbo);glDeleteTextures(1,&source);gfx_tex_esquecer(0);
  }
  sw=1920;sh=1080;iw=1280;ih=720;
  pretendUtgard=1;gpun_iniciar(sw,sh);gpun_definir_nivel(3);
  gpun_quadro_inicio();frame(0);
  glGenTextures(1,&source);glBindTexture(GL_TEXTURE_2D,source);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,iw,ih,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,iw,ih);gfx_tex_esquecer(0);
  gpun_quadro_fim();readback();memcpy(linear,pixels,(size_t)sw*sh*4);
  copy(0);readback();assert(!memcmp(linear,pixels,(size_t)sw*sh*4));
  pretendUtgard=0;glDeleteTextures(1,&source);gfx_tex_esquecer(0);
  puts("PASS Mali-450 automatically selects exact bilinear output");
  gpun_iniciar(1280,720);gpun_forcar_720();gfx_novo_quadro();gpun_quadro_inicio();
  GLint bound;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&bound);assert(bound==0);
  gpun_quadro_fim();assert(gfx_n_rect==0);
  gpun_iniciar(3840,2160);gpun_quadro_inicio();glGetIntegerv(GL_FRAMEBUFFER_BINDING,&bound);assert(bound==0);
  puts("PASS native 720/4K bypass, all production targets and alpha");
  free(pixels);free(flipped);free(linear);free(native);
  return failed;
}
