// Native Mac renderer fixture: actual Home geometry, text metrics and drawing.
#include "../src/home.c"
#include "dados.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static SDL_Window *window;
static CatItem items[12];
static void setup(int split, int glass) {
  char path[900];
  snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
  FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "idioma 1\nhomeLayoutLocal 2\nvidroLocal %d\nselected_theme 0\n", glass ? 0 : 1);
  fclose(f); ajustes_dir(getenv("NUVIO_DADOS"));
  memset(fileiras, 0, sizeof fileiras);
  nFileiras = 2; focoHero = 0; foco.fileira = foco.coluna = 0;
  fileiras[0].tipo = split ? FILEIRA_HEROI_DIVIDIDO : FILEIRA_HEROI;
  fileiras[0].n = 8; fileiras[1].tipo = FILEIRA_NORMAL;
  snprintf(fileiras[0].titulo, sizeof fileiras[0].titulo, "%s", split ? "Top Rated - Movie" : "Horror - Movie");
  scrollX[0] = alvoScrollFil(0, 0, 0, 0);
}
static void capture(const char *prefix, int split, int glass, float scale) {
  char path[1000];
  setup(split, glass); gfx_escala_txt_definir(scale);
  // Home keeps art geometry in screen coordinates; copy uses scaled metrics.
  float y = split ? 137 : 160;
  for (int frame = 0; frame < 50; frame++) {
    SDL_PumpEvents(); tex_bombear(8); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    txt_desenhar(txt_linha(TXT_ROW_TITULO, fileiras[0].titulo, 243,242,239,255), 88, y - 62);
    if (split) {
      float w = larguraFil(0), h = alturaFil(0);
      for (int c = 1; c < 5; c++) {
        float x = homeFileiraX(0,c) + xOffTipo(FILEIRA_HEROI_DIVIDIDO);
        CartaoFoco k = {.r=0,.c=c,.tipo=FILEIRA_HEROI_DIVIDIDO,.cItem=&items[c],
          .caminho=items[c].poster,.t=tex_obter_larg(items[c].poster,w),.aArte=1,
          .rotuloFora=1,.esc=1,.px=x,.py=y,.w=w,.h=h};
        pintarCartao(&k, raioDe(w,h));
      }
      desenhaDividido(0,y);
      GfxRect p=divididoPainel(0,y);
      gfx_rect((GfxRect){p.x-4,p.y-4,p.w+8,p.h+8},0,GFX_ANEL,0,4/(p.h+8),0,raioDe(p.w+8,p.h+8),1,1,1,1);
    } else {
      CartaoFoco k = {.r=0,.tipo=FILEIRA_HEROI,.cItem=&items[0],.caminho=items[0].backdrop,
        .t=tex_obter_larg(items[0].backdrop,1744),.aArte=1,.esc=1,.f=1,.px=homeConteudoX(),.py=y,
        .w=larguraFil(0),.h=alturaFil(0)};
      pintarCartao(&k,raioDe(k.w,k.h));
    }
    float next = y + alturaTotalFil(0) + fileiraGap();
    txt_desenhar(txt_linha(TXT_ROW_TITULO,"Latest Horror - Movie",155,155,158,255),88,next);
    for(int c=0;c<8;c++) {
      GfxRect r={88+c*243.0f,next+64,212,318};
      desenhaArteCard(r,FILEIRA_NORMAL,items[c].poster,tex_obter_larg(items[c].poster,212),&items[c],0,raioDe(r.w,r.h),1,0);
    }
    if(frame==49) {
      unsigned char *pixels=malloc(1920*1080*4); assert(pixels);
      SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,1920,1080,32,SDL_PIXELFORMAT_RGBA32); assert(s);
      glReadPixels(0,0,1920,1080,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
      for(int row=0;row<1080;row++) memcpy((char*)s->pixels+row*s->pitch,pixels+(1079-row)*1920*4,1920*4);
      snprintf(path,sizeof path,"%s-%s%s.png",prefix,split?"heroidiv":"heroicard",glass?"":"-semvidro");
      assert(IMG_SavePNG(s,path)==0); SDL_FreeSurface(s); free(pixels);
      printf("PNG %s; rects=%d fill=%.3f\n",path,gfx_n_rect,gfx_fill);
    }
    SDL_GL_SwapWindow(window); SDL_Delay(4);
  }
}
int main(int argc,char **argv) {
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  window=SDL_CreateWindow("Hero A proof",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN); assert(window);
  assert(SDL_GL_CreateContext(window)); SDL_GL_SetSwapInterval(0); glViewport(0,0,1920,1080);
  gfx_tamanho_alvo(1920,1080); assert(gfx_iniciar()); assert(txt_iniciar("deploy/app",1)); tex_iniciar(64);
  dados_iniciar("deploy/app/art"); ajustes_iniciar(); artehero_definir_falhou(tex_falhou);
  for(int i=0;i<12;i++) {
    snprintf(items[i].imdb,sizeof items[i].imdb,"tt900%04d",i);
    snprintf(items[i].tipo,sizeof items[i].tipo,"movie");
    snprintf(items[i].titulo,sizeof items[i].titulo,"The Further");
    snprintf(items[i].meta,sizeof items[i].meta,"2025 · 1h 47min");
    snprintf(items[i].genero,sizeof items[i].genero,"Horror, Mystery"); items[i].nota=68;
    snprintf(items[i].sinopse,sizeof items[i].sinopse,"Gemma, a young mother raising her daughter in the house she grew up in, discovers she can travel into The Further, where she possesses an ability to bring what lives there back to the real world.");
    snprintf(items[i].backdrop,sizeof items[i].backdrop,"deploy/app/art/%02d.jpg",i);
    snprintf(items[i].poster,sizeof items[i].poster,"deploy/app/art/poster/%02d.jpg",i);
    snprintf(items[i].logo,sizeof items[i].logo,"deploy/app/art/logo/%02d.png",i%10);
  }
  CatFileira cf={.n=12}; cat_definir_tudo(items,12,&cf,1);
  if(argc>1 && argv[1][0]!='-') for(int split=0;split<(argc>2 ? 1 : 2);split++) for(int glass=0;glass<2;glass++) capture(argv[1],split,glass,1);
  setup(0,0);
  assert(fabsf(alturaFil(0)/larguraFil(0)-372.0f/1744)<0.0001f);
  assert(fabsf(larguraFil(0)-1712)<1);
  assert(fabsf(alturaTotalFil(0)-alturaFil(0))<0.01f);
  if((argc>1 && !strcmp(argv[1],"--card")) || argc>2) { puts("PASS heroicard A"); return 0; }
  setup(1,0);
  float w=larguraFil(0),h=alturaFil(0),right=homeFileiraX(0,3)+xOffTipo(FILEIRA_HEROI_DIVIDIDO)+w;
  assert(right<=1832.01f && fabsf(right-homeConteudoDireita())<0.01f);
  assert(fabsf(w/h-2.0f/3)<0.001f);
  assert(fabsf(gapDe(FILEIRA_HEROI_DIVIDIDO)-24)<0.01f);
  assert(fabsf(alturaTotalFil(0)-h-150)<0.01f);
  for(int theme=0;theme<2;theme++) for(int i=0;i<3;i++) {
    const float scales[]={0.8f,1,1.15f}; gfx_escala_txt_definir(scales[i]);
    TxtLinha mark=txt_linha(TXT_MINI,"IMDb",10,10,10,255),score=txt_linha(TXT_CAPTION2,"6.8",235,235,235,255);
    assert(badge_imdb_largura(68)>=mark.w+14+BADGE_IMDB_GAP+score.w);
  }
  puts("PASS hero A: card aspect, three flush posters, shared 150px meta band, measured IMDb padding; scales 80/100/115%");
  SDL_Quit(); return 0;
}
