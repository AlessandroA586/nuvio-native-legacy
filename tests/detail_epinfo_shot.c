// Reuse the offline series fixture and real GL renderer.
#include "tex_cache.h"
static int checkThumbnailWidth, thumbnailRequests;
static GLuint checkedThumbnail(const char *path, float width);
#define tex_obter_larg checkedThumbnail
#define main episode_fixture_main
#include "detail_eps_shot.c"
#undef main
#undef tex_obter_larg

static GLuint checkedThumbnail(const char *path, float width) {
  if (checkThumbnailWidth) {
    assert(width == epCardW());
    thumbnailRequests++;
  }
  return tex_obter_larg(path, width);
}

static void checkThumbnailHoles(void) {
  assert(!ajustes_borda_foco() && !ajustes_vidro());
  assert(animFoco[SEC_EPISODIOS][foco.coluna] > .999f);
  float settled = animFoco[SEC_EPISODIOS][foco.coluna];
  const float focus[] = {0, .5f, 1};
  for (int f=0;f<3;f++) {
    animFoco[SEC_EPISODIOS][foco.coluna] = focus[f];
    GfxRect holes[N_ITENS * 2];
    thumbnailRequests=0; checkThumbnailWidth=1;
    int n=epFuros(holes), expected=0;
    checkThumbnailWidth=0;
    assert(n > 0 && n == thumbnailRequests * 2);
    for (int c=0;c<epVisiveis() && c<N_ITENS && c<DET_REV_EP;c++) {
      float x=xItem(SEC_EPISODIOS,c)-scrollSec[SEC_EPISODIOS];
      if (x>NV_TELA_W || x+epCardW()<0) continue;
      float y=epRowY()-scrollY, inset=NV_DETP_EP_RAIO+2;
      GfxRect rects[] = {{x+inset,y+2,epCardW()-2*inset,epThumbH()-4},
                         {x+2,y+inset,epCardW()-4,epThumbH()-2*inset}};
      for (int i=0;i<2;i++) {
        assert(expected < n);
        GfxRect h=holes[expected++], r=rects[i];
        assert(h.x==r.x && h.y==r.y && h.w==r.w && h.h==r.h);
      }
    }
    assert(expected == n);
  }
  animFoco[SEC_EPISODIOS][foco.coluna] = settled;
  puts("PASS: fixed thumbnail holes and texture widths at rest and during focus changes");
}

static void saveShot(const char *path, int w, int h) {
  unsigned char *pixels = malloc((size_t)w * h * 4);
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
  assert(pixels && surface);
  glFinish();
  glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  for (int y=0;y<h;y++)
    memcpy((char *)surface->pixels+y*surface->pitch,pixels+(h-1-y)*w*4,w*4);
  assert(IMG_SavePNG(surface,path)==0);
  SDL_FreeSurface(surface); free(pixels);
  printf("PNG %s\n",path);
}

int main(int argc, char **argv) {
  assert(argc == 3);
  SDL_SetHint("SDL_MAC_BACKGROUND_APP","1");
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);
  IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  janela=SDL_CreateWindow("Episode information",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
  assert(janela);
  assert(SDL_GL_CreateContext(janela));
  glGenTextures(1,&fboTex); glBindTexture(GL_TEXTURE_2D,fboTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1920,1080,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
  glGenFramebuffers(1,&fbo); glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fboTex,0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
  gfx_tamanho_alvo(1920,1080);
  assert(gfx_iniciar()); assert(txt_iniciar("deploy/app",1));
  tex_iniciar(16); gfx_icones_dir("deploy/app/art");
  ajustes_iniciar(); dados_iniciar("deploy/app/art");

  // GL proof: focus adds no pixels inside the card, on dark or light art.
  for (int light=0;light<2;light++) {
    float background=light?.9f:.08f;
    glViewport(0,0,1920,1080); gfx_novo_quadro(); gfx_sem_recorte();
    glClearColor(background,background,background,1); glClear(GL_COLOR_BUFFER_BIT);
    unsigned char before[4],after[4];
    glReadPixels(400,700,1,1,GL_RGBA,GL_UNSIGNED_BYTE,before);
    epFocoDesenha((GfxRect){100,100,640,548},1,1);
    glReadPixels(400,700,1,1,GL_RGBA,GL_UNSIGNED_BYTE,after);
    assert(memcmp(before,after,4)==0);
    unsigned char edge[640*4]; int dots=0,gaps=0;
    glReadPixels(100,978,640,1,GL_RGBA,GL_UNSIGNED_BYTE,edge);
    for(int x=40;x<600;x++) {
      dots+=edge[x*4]>245;
      gaps+=abs(edge[x*4]-before[0])<3;
    }
    assert(dots>10 && gaps>10);
  }
  puts("PASS: GL dotted focus has transparent interior on dark/light artwork");

  // Preserve image detail while exercising both luminance extremes.
  for (int light=0;light<2;light++) {
    char art[700];
    snprintf(art,sizeof art,"%s/%s.png",dados_dir(),light?"light":"dark");
    SDL_Surface *source=IMG_Load("deploy/app/art/03.jpg"); assert(source);
    SDL_Surface *image=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_RGBA32,0); assert(image);
    SDL_FreeSurface(source);
    for(int y=0;y<image->h;y++) for(int x=0;x<image->w;x++) {
      unsigned char *p=(unsigned char *)image->pixels+y*image->pitch+x*4;
      for(int c=0;c<3;c++) p[c]=light ? 190+p[c]/4 : p[c]/3;
    }
    assert(IMG_SavePNG(image,art)==0); SDL_FreeSurface(image);
    for(int layout=0;layout<3;layout+=2) {
      char settings[700]; snprintf(settings,sizeof settings,"%s/ajustes.txt",dados_dir());
      FILE *file=fopen(settings,"w"); assert(file);
      fprintf(file,"idioma 1\nhomeLayoutLocal %d\nbordaFocoCartaz 1\nvidroLocal 1\n",layout); fclose(file);
      ajustes_dir(dados_dir());
      montarCatalogo();
      snprintf(itens[0].backdrop,sizeof itens[0].backdrop,"%s",art);
      itens[0].temporada=1; itens[0].episodio=4; itens[0].progresso=45;
      cat_definir_tudo(itens,2,&filEnsaio,1);
      for(int i=0;i<T1_N;i++) snprintf(episodios[i].thumb,sizeof episodios[i].thumb,"%s",art);
      cat_definir_episodios(0,episodios,T1_N+T2_N+T3_N);
      semear(1,T1_N,3); abrir(0,0); nosEpisodios(3);
      parado=1; quadros(32);
      checkThumbnailHoles();
      for(int h=1080;h>=720;h-=360) {
        int w=h*16/9;
        gfx_tamanho_alvo(w,h); glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glViewport(0,0,w,h); gfx_novo_quadro(); txt_novo_quadro();
        gfx_sem_recorte(); glClearColor(.02f,.02f,.025f,1); glClear(GL_COLOR_BUFFER_BIT);
        detail_desenhar(10000);
        char path[900];
        snprintf(path,sizeof path,"%s/%s-%s-%dp-%s.png",argv[1],argv[2],
                 layout?"refined":"modern",h,light?"light":"dark");
        saveShot(path,w,h);
      }
      gfx_tamanho_alvo(1920,1080);
    }
  }
  return 0;
}
