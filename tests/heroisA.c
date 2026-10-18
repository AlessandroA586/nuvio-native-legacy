// Native Mac renderer fixture: actual Home geometry, text metrics and drawing.
#include <string.h>
#include "../src/gfx.h"
#include "../src/text.h"
static float headingY;
static GLuint headingTexture;
static int overlayProbe;
static const char *heading;
static unsigned renderTime = 1000;
static TxtLinha recordLine(TxtEstilo style, const char *s, int r, int g, int b, int a, float width) {
  TxtLinha l = txt_linha_corta(style,s,r,g,b,a,width);
  if (style == TXT_ROW_TITULO) headingTexture = l.tex;
  return l;
}
static void recordText(TxtLinha l, float x, float y) {
  if (l.tex == headingTexture && headingY < 0) headingY = y;
  txt_desenhar(l, x, y);
}
static void recordRect(GfxRect r, GLuint tex, GfxModo mode, float focus,
                       float px, float py, float radius,
                       float red, float green, float blue, float alpha) {
  if (overlayProbe == 1 && r.x == 0 && r.y == 0 && r.h <= 96 &&
      (mode == GFX_VEU_CSS || mode == GFX_COR)) {
    gfx_cor((GfxRect){0, 0, r.w, 96}, 0, 1, 1, 1, 1);
    overlayProbe = 2;
  }
  gfx_rect(r, tex, mode, focus, px, py, radius, red, green, blue, alpha);
}
static void recordColor(GfxRect r, float radius, float red, float green, float blue, float alpha) {
  if (overlayProbe == 1 && r.x == 0 && r.y == 0 && r.h <= 96) {
    gfx_cor((GfxRect){0,0,r.w,96},0,1,1,1,1); overlayProbe = 2;
  }
  gfx_cor(r,radius,red,green,blue,alpha);
}
#define txt_linha_corta recordLine
#define gfx_cor recordColor
#define txt_desenhar recordText
#define gfx_rect recordRect
#include "../src/home.c"
#undef txt_linha_corta
#undef gfx_cor
#undef txt_desenhar
#undef gfx_rect
#include "dados.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static int budgetFailed;
static SDL_Window *window;
static CatItem items[12];
static void setup(int split, int glass) {
  char path[900];
  snprintf(path, sizeof path, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
  FILE *f = fopen(path, "w"); assert(f);
  fprintf(f, "idioma 1\nhomeLayoutLocal 2\nmodernLandscapePostersEnabled 1\nbordaFocoCartaz 0\nvidroLocal %d\nselected_theme %d\n", glass ? 0 : 1,
          getenv("NV_HEROIS_TEMA") ? atoi(getenv("NV_HEROIS_TEMA")) : 0);
  fclose(f); ajustes_dir(getenv("NUVIO_DADOS"));
  memset(fileiras, 0, sizeof fileiras);
  nFileiras = 2; focoHero = 0; foco.fileira = foco.coluna = 0;
  fileiras[0].tipo = split ? FILEIRA_HEROI_DIVIDIDO : FILEIRA_HEROI;
  fileiras[0].n = 8; fileiras[1].tipo = FILEIRA_NORMAL;
  snprintf(fileiras[0].titulo, sizeof fileiras[0].titulo, "%s", split ? "Top Rated - Movie" : "Horror - Movie");
  scrollX[0] = alvoScrollFil(0, 0, 0, 0);
  animFoco[0][0] = animFocoFil[0] = 1;
  animFocoFil[1] = 0;
  snprintf(fileiras[1].titulo, sizeof fileiras[1].titulo, "Latest Horror - Movie");
  fileiras[1].n = 8;
  scrollY = dinRolagemCentrada(0, 0);
  fileirasVistasEm = 1000;
  filNAntes[0] = filNAntes[1] = 8;
  memset(filEntraEm, 0, sizeof filEntraEm);
}
static void renderHome(int split, int probe) {
  heading = fileiras[foco.fileira].titulo; headingTexture = 0;
  headingY = -1; overlayProbe = probe;
  home_desenhar(renderTime);
  if (probe) {
    unsigned char pixel[4];
    glReadPixels(10,1080-35,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(overlayProbe == 2);
    assert(abs(pixel[0]-(int)(NV_COR_FUNDO_R*255)) <= 2);
    glReadPixels(10,1080-71,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0] > NV_COR_FUNDO_R*255+20 && pixel[0] < 245);
    glReadPixels(10,1080-96,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    assert(pixel[0] > 245);
    puts("PASS top overlay: solid through 45%, then fades to 96px");
  } else {
    if (fabsf(headingY-(split ? 78 : 98))>=0.01f)
      fprintf(stderr,"heading=%g split=%d row=%d layout=%d scroll=%g\n",headingY,split,foco.fileira,layoutHome(),scrollY);
    assert(fabsf(headingY-(split ? 78 : 98))<0.01f);
    assert(temItemFoco && fabsf(itemFoco.rect.y-(split ? 137 : 160))<0.01f);
  }
}
static void capture(const char *prefix, int split, int glass, float scale) {
  char path[1000];
  setup(split, glass); gfx_escala_txt_definir(scale); TXT_INI();
  scrollX[0] = alvoScrollFil(0, 0, 0, 0);

  for (int frame = 0; frame < 50; frame++) {
    renderTime = 1000 + frame * 16;
    SDL_PumpEvents(); tex_bombear(8); tex_novo_quadro(); txt_novo_quadro(); gfx_novo_quadro();
    glClearColor(NV_COR_FUNDO_R, NV_COR_FUNDO_G, NV_COR_FUNDO_B, 1); glClear(GL_COLOR_BUFFER_BIT);
    renderHome(split, 0);
    if(frame==49) {
      unsigned char *pixels=malloc(1920*1080*4); assert(pixels);
      SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,1920,1080,32,SDL_PIXELFORMAT_RGBA32); assert(s);
      glReadPixels(0,0,1920,1080,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
      for(int row=0;row<1080;row++) memcpy((char*)s->pixels+row*s->pitch,pixels+(1079-row)*1920*4,1920*4);
      snprintf(path,sizeof path,"%s-%s%s.png",prefix,split?"heroidiv":"heroicard",glass?"":"-semvidro");
      assert(IMG_SavePNG(s,path)==0); SDL_FreeSurface(s); free(pixels);
      printf("PNG %s; rects=%d fill=%.3f gpu=%.3f\n",path,gfx_n_rect,gfx_fill,gfx_fill_gpu);
      if (!split && scale == 1.0f) {
        // Isolate the reviewed card from unrelated background/row submissions.
        gfx_novo_quadro();
        const CatItem *ci = cat_item(0);
        GfxRect r = itemFoco.rect;
        desenhaHeroiCard(ci,1,ci->sinopse,ci->backdrop,
                        tex_obter_larg(ci->backdrop,r.w),1,
                        r.x,r.y,r.w,r.h,raioDe(r.w,r.h),1,0);
        double limit = 0.75; // Artwork + focus ring + copy; no separate veil area.
        printf("GPU card glass=%d fill=%.3f limit=%.3f\n",glass,gfx_fill,limit);
        if (gfx_fill > limit + 0.0005) {
          fprintf(stderr, "FAIL wide hero GPU budget: glass=%d fill=%.3f limit=%.3f\n",
                  glass, gfx_fill, limit);
          budgetFailed = 1;
        }
      }
    }
    SDL_GL_SwapWindow(window); SDL_Delay(4);
  }
  TXT_FIM();
}
int main(int argc,char **argv) {
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0); IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
  window=SDL_CreateWindow("Hero A proof",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN); assert(window);
  assert(SDL_GL_CreateContext(window)); SDL_GL_SetSwapInterval(0); glViewport(0,0,1920,1080);
  gfx_tamanho_alvo(1920,1080); assert(gfx_iniciar()); assert(txt_iniciar("deploy/app",1)); tex_iniciar(64);
  dados_iniciar("deploy/app/art");
  // Exercise explicit fixture settings, not the first-run appearance migration.
  dados_gravar("aparencia-20.txt", "1\n");
  AddonRemoto addon = {.nome="Fixture",.url="https://fixture.invalid/manifest.json",.ativo=1};
  addons_definir_lista(&addon,1); assert(addons_n()==1);
  ajustes_iniciar(); artehero_definir_falhou(tex_falhou);
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
  // Warm local artwork before measuring settled frames, never loading placeholders.
  for (int frame=0;frame<200;frame++) {
    int ready=1;
    tex_pedido_fora(0); tex_bombear(24); tex_novo_quadro();
    for (int i=0;i<8;i++) {
      if (!tex_obter_larg(items[i].backdrop,1744)) ready=0;
      if (!tex_obter_larg(items[i].poster,400)) ready=0;
    }
    if (ready) break;
    SDL_Delay(4);
  }
  assert(tex_obter_larg(items[0].backdrop,1744));
  if(argc>1 && !strcmp(argv[1],"--fade")) {
    setup(0,0); renderHome(0,1); SDL_Quit(); return 0;
  }
  for(int split=0;split<2;split++) {
    setup(split,0); renderHome(split,0);
    Fileira hero = fileiras[0];
    fileiras[0] = fileiras[1]; fileiras[1] = hero;
    snprintf(fileiras[0].titulo,sizeof fileiras[0].titulo,"Previous row");
    foco.fileira = 1;
    scrollX[1] = alvoScrollFil(1,0,0,0);
    scrollY = dinRolagemCentrada(1,NV_LEGACY_ROW_HEAD_H+alturaTotalFil(0)+fileiraGap());
    renderHome(split,0);
  }
  setup(0,0); renderHome(0,1);
  if(argc>1 && argv[1][0]!='-') {
    for(int split=0;split<(argc>2 ? 1 : 2);split++) for(int glass=0;glass<2;glass++) capture(argv[1],split,glass,1);
    if(argc==2) for(int i=0;i<2;i++) {
      char scaled[1000]; const float scales[]={0.8f,1.15f};
      snprintf(scaled,sizeof scaled,"%s-s%d",argv[1],i ? 115 : 80);
      for(int split=0;split<2;split++) capture(scaled,split,0,scales[i]);
    }
  }
  if (argc == 1) for (int glass=0;glass<2;glass++)
    capture(getenv("NUVIO_DADOS"),0,glass,1);
  assert(!budgetFailed);
  puts("PASS wide hero GPU budget: isolated card <=0.75 screens in both glass modes");
  gfx_escala_txt_definir(1);
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
    const float scales[]={0.8f,1,1.15f};
    setenv("NV_HEROIS_TEMA",theme ? "2" : "0",1);
    setup(1,0); gfx_escala_txt_definir(scales[i]); TXT_INI();
    assert(fabsf(gfx_escala_tela_txt()-scales[i])<0.001f);
    float sw=larguraFil(0),sh=alturaFil(0);
    for(int col=0;col<5;col++) {
      foco.coluna=col; scrollX[0]=alvoScrollFil(0,col,0,0);
      for(int c=col+1;c<=col+4;c++) {
        float x=homeFileiraX(0,c)+xOffTipo(FILEIRA_HEROI_DIVIDIDO);
        assert(divididoCartazVisivel(0,x+sw*0.5f,sw)==(c<=col+3));
      }
    }
    // Both edge posters must remain drawable throughout Right and Left motion.
    for(int col=0;col<4;col++) {
      float from=alvoScrollFil(0,col,0,0),to=alvoScrollFil(0,col+1,0,0);
      foco.coluna=col+1;
      for(int frame=1;frame<4;frame++) {
        scrollX[0]=from+(to-from)*frame/4.0f;
        for(int c=col+1;c<=col+4;c++) {
          float x=homeFileiraX(0,c)+xOffTipo(FILEIRA_HEROI_DIVIDIDO);
          assert(divididoCartazVisivel(0,x+sw*0.5f,sw));
        }
      }
      foco.coluna=col;
      for(int frame=1;frame<4;frame++) {
        scrollX[0]=to+(from-to)*frame/4.0f;
        for(int c=col+1;c<=col+4;c++) {
          float x=homeFileiraX(0,c)+xOffTipo(FILEIRA_HEROI_DIVIDIDO);
          assert(divididoCartazVisivel(0,x+sw*0.5f,sw));
        }
      }
    }
    assert(fabsf(sw/sh-2.0f/3)<0.001f);
    TxtLinha mark=txt_linha(TXT_MINI,"IMDb",10,10,10,255),score=txt_linha(TXT_CAPTION2,"6.8",235,235,235,255);
    assert(badge_imdb_largura(68)>=mark.w+14+BADGE_IMDB_GAP+score.w);
    assert(fabsf(badge_imdb(0,0,68,0,0)-badge_imdb_largura(68))<0.01f);
    float mh=heroiMeta(&items[0],104,600,divPainelW());
    float synopsis=sinopse_bloco_corta(TXT_DET_SIN,items[0].sinopse,243,242,239,104,650,divPainelW(),36,0,2);
    assert(18+mh+14+synopsis<=NV_DIV_META_H);
    TXT_FIM();
  }
  puts("PASS hero A: card aspect, three flush posters, shared 150px meta band, measured IMDb padding; scales 80/100/115%");
  SDL_Quit(); return 0;
}
