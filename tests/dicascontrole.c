// Real What's New dismissal, Settings action, persistence across processes,
// and the existing hidden SDL/GL + FBO screenshot harness.
#include "../src/ajustes.c"
#include "dicascontrole.h"
#include "app.h"
#include "novidades_cartao.h"
#include "novidades20.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static void key(SDL_Keycode k, void (*evento)(const SDL_Event *)) {
  SDL_Event e = {0}; e.type = SDL_KEYDOWN; e.key.keysym.sym = k; evento(&e);
}
static void settle(void) { for (int i = 0; i < 40; i++) dicascontrole_atualizar(1.f/60, SDL_GetTicks()); }
static void settings(void) {
  int item = -1;
  ajustes_iniciar();
  for (int i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == AJ_DICASCONTROLE) item = i;
  assert(item >= 0 && visivel(item) && focavel(item));
  assert(!strcmp(uxCaminho(AJ_DICASCONTROLE), "Sobre e ajuda"));
  ajPontLista = 1; ajPonteiroItem(item, 0);
  assert(focoOp == AJ_DICASCONTROLE);
  key(SDLK_RETURN, ajustes_evento);
  assert(ajustes_pediu_dicascontrole());
  assert(!ajustes_pediu_dicascontrole());
  dicascontrole_abrir(); assert(dicascontrole_aberto());
}
static void state(int first) {
  assert(!dicascontrole_ocupado());
  dicascontrole_atualizar(1, 0); assert(!dicascontrole_aberto());
  dados_gravar("novidades-20-guia.txt", "1\n");
  if (first) {
    novcartao_decidir(1, 0, 0); assert(novcartao_aberto());
    assert(!dicascontrole_ocupado());
    key(SDLK_ESCAPE, novcartao_evento);
    assert(!novcartao_aberto() && dicascontrole_ocupado() && !dicascontrole_aberto());
    dicascontrole_atualizar(.18f, 0); assert(!dicascontrole_aberto());
    settle(); assert(dicascontrole_aberto() && !app_central_pode());
    assert(dicascontrole_pagina() == 0);
    key(SDLK_RIGHT, app_evento); assert(dicascontrole_pagina() == 1);
    key(SDLK_LEFT, app_evento); assert(dicascontrole_pagina() == 0);
    key(SDLK_LEFT, app_evento); assert(dicascontrole_pagina() == 0);
    for (int i = 0; i < 8; i++) key(SDLK_RIGHT, app_evento);
    assert(dicascontrole_pagina() == 3);
    key(SDLK_RETURN, app_evento); assert(!dicascontrole_aberto());
    settle();
    char *s = dados_ler(DICASCONTROLE_ARQUIVO); assert(s && !strcmp(s, "1\n")); free(s);
  } else {
    novcartao_decidir(1, 0, 0); assert(!novcartao_aberto());
    // Explicit replay of What's New also must not automatically repeat tips.
    novcartao_abrir(); key(SDLK_ESCAPE, novcartao_evento);
    settle(); assert(!dicascontrole_ocupado());
  }
  for (int i = 0; i < 3; i++) { dicascontrole_depois_novidades(); settle(); assert(!dicascontrole_ocupado()); }
  settings(); key(SDLK_ESCAPE, app_evento); settle(); assert(!dicascontrole_ocupado());
  for (int m = 0; m < 4; m++) {
    dicascontrole_teste_modo(m);
    assert(!strcmp(dicascontrole_tecla(1), m == DICAS_TV_CH ? "CH-" : m == DICAS_TOQUE ? "Buscar" : "Amarelo"));
  }
  if (first) {
    // Fresh installs show the 2.0 guide instead of the update card.
    dados_apagar(DICASCONTROLE_ARQUIVO);
    novidades20_abrir(0);
    key(SDLK_ESCAPE, novidades20_evento); key(SDLK_RIGHT, novidades20_evento); key(SDLK_RETURN, novidades20_evento);
    assert(!novidades20_aberto() && dicascontrole_ocupado());
    settle(); assert(dicascontrole_aberto());
    key(SDLK_ESCAPE, dicascontrole_evento); settle();
  }
  puts("PASS: dismissal, page bounds, modal routing, Settings replay and persistent marker");
}
static GLuint fbo, tex;
static int width = 1920, height = 1080;
static void backdrop(Uint32 now) {
  (void)now;
  gfx_cor((GfxRect){0,0,NV_TELA_W,NV_TELA_H}, 0, .06f,.07f,.1f,1);
  // Visible blocks behind the glass demonstrate the cached blur.
  for (int i = 0; i < 7; i++) {
    GfxRect card = {NV_TELA_W*.08f+i*NV_TELA_W*.13f,NV_TELA_H*.25f,NV_TELA_W*.11f,NV_TELA_H*.5f};
    gfx_cor(card, .03f, .16f+i*.018f, .13f, .20f+i*.01f, 1);
  }
}
static void frame(void) {
  SDL_PumpEvents(); txt_novo_quadro(); gfx_novo_quadro();
  ponteiro_quadro(SDL_GetTicks());
  glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0,0,width,height);
  glClearColor(.05f,.05f,.06f,1); glClear(GL_COLOR_BUFFER_BIT);
  dicascontrole_atualizar(1.f/60,SDL_GetTicks());
  dicascontrole_fundo(backdrop,0); dicascontrole_desenhar(0);
  ponteiro_desenhar(); glFinish();
}
static void capture(const char *out, const char *name) {
  char path[900]; snprintf(path,sizeof path,"%s/%s.png",out,name);
  unsigned char *pix=malloc(width*height*4); assert(pix);
  glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pix);
  SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_RGBA32); assert(s);
  for(int y=0;y<height;y++) memcpy((char*)s->pixels+y*s->pitch,pix+(height-1-y)*width*4,width*4);
  assert(IMG_SavePNG(s,path)==0); SDL_FreeSurface(s); free(pix);
  printf("PNG: %s\n",path);
}
static void finger(Uint32 type, float x, float y) {
  SDL_Event e = {0}; e.type = type; e.tfinger.touchId = 1; e.tfinger.fingerId = 1;
  e.tfinger.x = x/NV_TELA_W; e.tfinger.y = y/NV_TELA_H;
  assert(ponteiro_evento(&e, app_evento));
}
static void touch(void) {
  // The desktop harness discovers touch on the first finger, unlike Android.
  finger(SDL_FINGERDOWN,40,40); ponteiro_cancelar_toque(); finger(SDL_FINGERUP,40,40);
  dicascontrole_abrir(); for(int i=0;i<40;i++) frame();
  ponteiro_teste_janela(width,height);
  // Dot 3 selects a page directly; swipe right goes back once.
  float y=(NV_TELA_H-1100)*.5f+1100-149;
  finger(SDL_FINGERDOWN,NV_TELA_W*.5f+12,y);
  finger(SDL_FINGERUP,NV_TELA_W*.5f+12,y);
  assert(dicascontrole_pagina()==2);
  for(int i=0;i<40;i++) frame();
  finger(SDL_FINGERDOWN,300,900); finger(SDL_FINGERMOTION,600,900); finger(SDL_FINGERUP,600,900);
  assert(dicascontrole_pagina()==1);
  for(int i=0;i<40;i++) frame();
  // Fixed Done button remains tappable on a portrait display.
  y=(NV_TELA_H-1100)*.5f+1100-78;
  finger(SDL_FINGERDOWN,NV_TELA_W*.5f,y); finger(SDL_FINGERUP,NV_TELA_W*.5f,y);
  assert(!dicascontrole_aberto()); settle();
  puts("PASS: page dots, horizontal swipe and Done use real touch dispatch");
}
static void shots(const char *out) {
  assert(SDL_InitSubSystem(SDL_INIT_VIDEO)==0); IMG_Init(IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  SDL_Window *w=SDL_CreateWindow("remote-tips-shot",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN); assert(w);
  SDL_GLContext gl=SDL_GL_CreateContext(w); assert(gl);
  glGenTextures(1,&tex); glGenFramebuffers(1,&fbo);
  assert(gfx_iniciar()); assert(txt_iniciar("deploy/app",1));
  ponteiro_iniciar(); gfx_icones_dir("deploy/app/art");
  static const char *const modes[]={"android-tpk","webos","wgt","touch"};
  for (int m=0;m<4;m++) {
    dicascontrole_teste_modo(m);
    width=m==DICAS_TOQUE?1080:1920; height=m==DICAS_TOQUE?1920:1080;
    layout_modo_definir(m==DICAS_TOQUE); layout_tela_definir(width,height);
    gfx_tamanho_alvo(width,height); gfx_snap_encerrar(); assert(gfx_snap_iniciar(width,height));
    glBindTexture(GL_TEXTURE_2D,tex); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    gfx_tex_esquecer(0);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo); glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    for (int idi=0;idi<IDIOMA_N;idi++) {
      valor[AJ_IDIOMA]=idi+1; assert(ajustes_idioma()==idi);
      dicascontrole_abrir();
      for (int p=0;p<4;p++) {
        for(int i=0;i<50;i++) frame();
        // Translation expansion must fit above pagination on all platforms.
        const char *title=m==DICAS_TOQUE && p==2?"Segure para opções":
          m==DICAS_TOQUE? (const char*[]){"Painel de Salvos","Spotlight","Segure para opções","Barra lateral"}[p]:
          (const char*[]){"Painel de Salvos","Spotlight","Central de controle","Segurar OK num cartaz"}[p];
        float tw=m==DICAS_TOQUE?888:1440*.53f-64;
        float th=txt_bloco_corta(TXT_ILHA_TITULO,i18n(title),243,242,239,0,0,tw,48,0,0);
        const char *body=m==DICAS_TOQUE?(const char*[]){"Toque no relógio para abrir Salvos e Avisos.","Segure Buscar no menu para abrir o Spotlight.","Toque e segure um cartaz para abrir o menu.","Deslize da borda esquerda para abrir o menu."}[p]:
          (const char*[]){"CH+ abre Salvos e Avisos na lateral direita.","Buscar filmes, séries, pessoas e canais","Segure CH+ e a ilha vira um painel de ajustes rápidos, com atalhos que você escolhe.","Segure OK para opções"}[p];
        float bh=txt_bloco_corta(TXT_BODY,i18n(body),243,242,239,0,0,tw,38,0,0);
        assert(th+bh<(m==DICAS_TOQUE?293:276));
        if (idi==0 || idi==IDIOMA_DE || idi==IDIOMA_JA) {
          char name[160]; snprintf(name,sizeof name,"%s-%s-page-%d",modes[m],idi==0?"pt":idi==IDIOMA_DE?"de":"ja",p+1); capture(out,name);
        }
        key(SDLK_RIGHT,dicascontrole_evento);
      }
      key(SDLK_ESCAPE,dicascontrole_evento); settle();
    }
  }
  puts("PASS: all pages fit in 30 languages and four device modes");
  touch();
  width=1920; height=1080; layout_tela_definir(width,height); gfx_tamanho_alvo(width,height);
  gfx_snap_encerrar(); assert(gfx_snap_iniciar(width,height));
  glBindTexture(GL_TEXTURE_2D,tex); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL); gfx_tex_esquecer(0);
  dicascontrole_abrir(); valor[AJ_IDIOMA]=IDIOMA_PT+1; valor[AJ_ANIM]=1;
  for(int i=0;i<12;i++) frame(); capture(out,"touch-landscape-reduced");
  key(SDLK_RIGHT,dicascontrole_evento); frame();
  assert(dicascontrole_pagina()==1); capture(out,"touch-landscape-reduced-page-2");
  key(SDLK_ESCAPE,dicascontrole_evento); frame(); assert(!dicascontrole_visivel());
  puts("PASS: reduced motion settles immediately in landscape");
  dicascontrole_encerrar(); gfx_snap_encerrar(); SDL_GL_DeleteContext(gl); SDL_DestroyWindow(w);
}
int main(int argc,char **argv) {
  setbuf(stdout,NULL); setbuf(stderr,NULL);
  assert(argc>=3); assert(SDL_Init(SDL_INIT_TIMER)==0);
  dados_iniciar(argv[1]); snprintf(dirAjustes,sizeof dirAjustes,"%s",argv[1]); ajustes_iniciar();
  if(!strcmp(argv[2],"shot")) { assert(argc==4); shots(argv[3]); }
  else state(!strcmp(argv[2],"first"));
  SDL_Quit(); return 0;
}
