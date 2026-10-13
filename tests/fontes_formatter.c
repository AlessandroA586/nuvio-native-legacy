// #479: real source sheet, hidden GL window, local fixtures only.
#include "streams.h"
#include "limpa.h"
static int cleanCalls;
static size_t countedClean(const char *s, char *d, size_t n, int flags) {
  cleanCalls++;
  return nv_limpar_texto(s, d, n, flags);
}
#define nv_limpar_texto countedClean
#include "../src/streams.c"
#undef nv_limpar_texto
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <unistd.h>
#define CARDS 60
#define MOVES 118
static Stream fixtures[CARDS];
static void frame(void) {
  txt_novo_quadro();
  stream_folha_atualizar(1.0f/60, SDL_GetTicks());
  stream_folha_desenhar(SDL_GetTicks());
}
static void setting(const char *dir, int addon) {
  char path[512]; snprintf(path,sizeof path,"%s/ajustes.txt",dir);
  FILE *f=fopen(path,"w"); assert(f);
  fprintf(f,"fonteTextoLocal %d\nvidroLocal 1\n",addon);
  fclose(f); ajustes_dir(dir);
  assert(ajustes_fonte_texto_addon()==addon);
}
static void measure(const char *dir, int addon, int verify) {
  setting(dir,addon);
  stream_definir_lista(fixtures,CARDS); stream_folha_abrir();
  anim=1;
  // Build every card's expanded text WITHOUT focus. Moving focus must reuse it.
  grupo=0;
  for(int row=0;row<CARDS;row++) {
    linhaAnt=row; abreAnt=1; montar(automaticaDaFolha()); rolagem=linhaY[row];
    for(int j=0;j<3;j++) { txt_novo_quadro(); stream_folha_desenhar(SDL_GetTicks()); }
  }
  grupo=1; foco=0; focoVisto=0; linhaAnt=-1; abreAnt=0; abreFoco=1;
  rolagem=0;
  // Warm shared selected labels without warming all selected card titles.
  for(int j=0;j<4;j++) frame();
  int raster=txt_rasterizadas, cleans=cleanCalls;
  Uint64 start=SDL_GetPerformanceCounter();
  for(int j=0;j<MOVES;j++) {
    SDL_Event e={0}; e.type=SDL_KEYDOWN;
    e.key.keysym.sym=j<CARDS-1?SDLK_DOWN:SDLK_UP;
    stream_folha_evento(&e); frame();
  }
  double ms=(SDL_GetPerformanceCounter()-start)*1000.0/SDL_GetPerformanceFrequency()/MOVES;
  printf("formatter=%d cards=%d moves=%d ms/move=%.3f rasterized=%d cleaned=%d\n",
         addon,CARDS,MOVES,ms,txt_rasterizadas-raster,cleanCalls-cleans);
  fflush(stdout);
  if(verify) {
    assert(txt_rasterizadas==raster && "focus must reuse prewarmed text textures");
    if(addon) assert(cleanCalls==cleans && "focus must reuse parsed addon cards");
  }
}
int main(int argc,char **argv) {
  int verify=argc==1 || strcmp(argv[1],"--measure");
  char dir[]="/tmp/nuvio-formatter479-XXXXXX"; assert(mkdtemp(dir));
  SDL_SetHint("SDL_MAC_BACKGROUND_APP","1");
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); IMG_Init(IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  SDL_Window *win=SDL_CreateWindow("formatter479",0,0,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN); assert(win);
  SDL_GLContext gl=SDL_GL_CreateContext(win); assert(gl); SDL_GL_SetSwapInterval(0);
  gfx_tamanho_alvo(1920,1080); assert(gfx_iniciar()); assert(txt_iniciar("deploy/app",1));
  glViewport(0,0,1920,1080);
  for(int i=0;i<CARDS;i++) {
    Stream *s=&fixtures[i]; s->altura=1080; s->mp4=1; s->fileIdx=-1;
    snprintf(s->url,sizeof s->url,"https://fixture.invalid/%d.mp4",i);
    snprintf(s->provedor,sizeof s->provedor,"AIOStreams");
    snprintf(s->rotulo,sizeof s->rotulo,"⚡ 𝗕𝗹𝘂𝗥𝗮𝘆 Source %02d",i);
    snprintf(s->descricao,sizeof s->descricao,
      "💾 11.1 GB | 30.1 Mbps |\n🔊 DDP5.1 Atmos\n🌐 English | Portuguese\n⚙️ Release group\nMovie.2026.1080p.Source.%02d.mkv",i);
  }
  measure(dir,0,verify); measure(dir,1,verify);
  // Append keeps existing parsed cards, including when the backing array moves.
  int cleans=cleanCalls;
  stream_lista_acrescentar(&fixtures[0],1,0);
  const TextoAddon *old=textoAddonDa(0); assert(old->nl==5);
  assert(cleanCalls==cleans);
  const TextoAddon *added=textoAddonDa(CARDS); assert(added->nl==5);
  assert(cleanCalls>cleans);
  cleans=cleanCalls; textoAddonDa(CARDS); assert(cleanCalls==cleans);
  // A replacement of the same length must discard the previous card content.
  snprintf(fixtures[0].rotulo,sizeof fixtures[0].rotulo,"Replacement");
  snprintf(fixtures[0].descricao,sizeof fixtures[0].descricao,"New description");
  stream_definir_lista(fixtures,CARDS);
  const TextoAddon *replaced=textoAddonDa(0);
  assert(!strcmp(replaced->titulo,"Replacement") && replaced->nl==1);
  // Settings switch while the list survives; parsing is independent of style.
  setting(dir,0); montar(automaticaDaFolha());
  setting(dir,1); montar(automaticaDaFolha());
  cleans=cleanCalls; montar(automaticaDaFolha()); assert(cleanCalls==cleans);
  puts("formatter cache: append, replacement and setting changes OK");
  txt_encerrar(); stream_definir_lista(NULL,0);
  SDL_GL_DeleteContext(gl); SDL_DestroyWindow(win); IMG_Quit(); SDL_Quit();
  char path[512]; snprintf(path,sizeof path,"%s/ajustes.txt",dir); unlink(path); rmdir(dir);
  return 0;
}
