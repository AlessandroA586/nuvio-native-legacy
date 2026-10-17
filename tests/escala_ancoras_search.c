#include "../src/busca.c"
#include "escala_ancoras.h"
void fixture_search(float scale) {
  static const char *names[]={"search-modern","search-standard","search-dynamic"};
  nFil=2; sugestao=0; painel=0; scrollY=0;
  memset(fil,0,sizeof fil); memset(animRes,0,sizeof animRes); memset(scrollX,0,sizeof scrollX);
  memset(filNova,0,sizeof filNova); memset(filEntraEm,0,sizeof filEntraEm);
  fil[0].titulo="Best result"; fil[0].melhor=1; fil[0].n=1;
  fil[1].titulo="Fixture results"; fil[1].n=5;
  for(int c=0;c<5;c++) fil[1].itens[c]=c;
  for(int frame=0;frame<14;frame++) { txt_novo_quadro(); tex_bombear(8); SDL_Delay(1); busca_desenhar(10000); }
  shot_begin(names[ajustes_home_layout()],scale); busca_desenhar(10000); shot_no_text_overlap(); shot_end();
  float old=gfx_escala_txt_entrar();
  shot_reset(); desenhaCampo(10000); GfxRect field=shot_text_bounds();
  GfxRect fieldBox=shot_box((GfxRect){buX(),buTopoEsq(),BU_COL_W,BU_CAMPO_H});
  shot_anchor("search field",fieldBox,0); shot_contains("field text inside",fieldBox,field);
  shot_reset(); buCabecalho(0,BU_RES_Y+4,BU_DIR-BU_RES_X); GfxRect heading=shot_text_bounds();
  shot_anchor("result section title",heading,0); shot_disjoint("field / results",field,heading);
  shot_reset(); desenhaResultados(10000); GfxRect results=shot_text_bounds();
  shot_contains("results within screen",(GfxRect){0,0,1920,1080},results);
  gfx_escala_sair(old);
}
