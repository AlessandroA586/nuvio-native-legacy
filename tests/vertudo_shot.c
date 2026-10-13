// CAPTURA DAS ABAS DA PAGINA DE COLECAO, nos dois estados, sem rede.
//
// Pedido do dono (19/09/2026): "vamos melhorar essas abas que nao estao
// legais, deixar mais parecido com o restante do app". As abas eram pilulas
// de 304x58 com anel, sublinhado e escala no foco — tres sinais que o resto do
// app ja abandonou (NV_COR_FOCO em layout.h; abas do painel de Salvos). Esta
// captura prova a gramatica nova:
//
//   com o D-pad nas abas:  a aba do cursor preenchida na cor de realce com
//                          texto escuro, a aberta em superficie clara;
//   com o D-pad na grade:  so a aberta clara, as outras quase transparentes.
//
// Inclui src/vertudo.c: `collection`, `tabFocus`, `tabCursor` e `source` sao
// estaticos, e semear por dentro e o unico jeito de fotografar sem addon.
// #359: a grade com titulos, sem rede. vertudo.c le os itens por
// desc_vertudo_n/desc_vertudo_item; com SHOT_GRADE>0 eles vem daqui.
static int shotGrade;
#define desc_vertudo_n shot_vertudo_n
#define desc_vertudo_item shot_vertudo_item
#include "../src/vertudo.c"
#undef desc_vertudo_n
#undef desc_vertudo_item
int desc_vertudo_n(void);
int desc_vertudo_item(int i, CatItem *dst);
int shot_vertudo_n(void) { return shotGrade ? shotGrade : desc_vertudo_n(); }
int shot_vertudo_item(int i, CatItem *dst) {
  if (!shotGrade) return desc_vertudo_item(i, dst);
  if (i < 0 || i >= shotGrade || !dst) return 0;
  memset(dst, 0, sizeof *dst);
  snprintf(dst->imdb, sizeof dst->imdb, "tt%07d", 100 + i);
  snprintf(dst->tipo, sizeof dst->tipo, "movie");
  snprintf(dst->titulo, sizeof dst->titulo, "Titulo %d", i + 1);
  snprintf(dst->genero, sizeof dst->genero, "Filme · Drama");
  snprintf(dst->meta, sizeof dst->meta, "%d · 120 min", 1992 + i * 3);
  snprintf(dst->sinopse, sizeof dst->sinopse, "Sinopse de exemplo para o painel da direita.");
  if (i == 1 || i == 5) dst->progresso = 45;   // em andamento ("Parou em")
  snprintf(dst->poster, sizeof dst->poster, "deploy/app/art/%02d.jpg", i % 30);
  snprintf(dst->backdrop, sizeof dst->backdrop, "deploy/app/art/%02d.jpg", i % 30);
  return 1;
}
#include "rail_shot.h"
#include <SDL2/SDL_image.h>
#include <assert.h>

static ColFolder pasta;
static ColSource pastaFontes[COL_SOURCE_MAX];   // a pasta so aponta (#255)

static void captura(const char *nome, SDL_Window *win) {
  int i;
  rail_shot_aplicar();
  for (i = 0; i < 90; i++) {
    SDL_PumpEvents();
    txt_novo_quadro();
    tex_novo_quadro();
    tex_bombear(6);
    gfx_novo_quadro();
    glClearColor(0.051f, 0.051f, 0.051f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    vertudo_atualizar(1.0f / 60.0f, SDL_GetTicks());
    vertudo_desenhar(SDL_GetTicks());
    rail_shot_desenhar(MENU_INICIO);
    if (i == 89) {
      unsigned char *pix = malloc(1920 * 1080 * 4);
      SDL_Surface *s;
      int y;
      assert(pix);
      glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, pix);
      s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_RGBA32);
      assert(s);
      for (y = 0; y < 1080; y++)
        memcpy((char *)s->pixels + y * s->pitch, pix + (1079 - y) * 1920 * 4, 1920 * 4);
      assert(SDL_SaveBMP(s, nome) == 0);
      SDL_FreeSurface(s);
      free(pix);
    }
    SDL_GL_SwapWindow(win);
  }
  printf("captura: %s\n", nome);
}

static void fonte(int i, const char *titulo, const char *tipo) {
  ColSource *s = &pasta.sources[i];
  snprintf(s->title, sizeof s->title, "%s", titulo);
  snprintf(s->type, sizeof s->type, "%s", tipo);
  snprintf(s->base, sizeof s->base, "https://exemplo.invalid/%d", i);
  snprintf(s->catId, sizeof s->catId, "cat%d", i);
}

int main(int argc, char **argv) {
  const char *saida = argc > 1 ? argv[1] : "/tmp/nuvio-vertudo";
  char nome[600];
  SDL_Window *w;
  pasta.sources = pastaFontes;
  SDL_GLContext gl;

  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  w = SDL_CreateWindow("Nuvio: abas da colecao", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1920, 1080,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  assert(w);
  gl = SDL_GL_CreateContext(w);
  assert(gl);
  SDL_GL_SetSwapInterval(0);
  glViewport(0, 0, 1920, 1080);
  gfx_tamanho_alvo(1920, 1080);
  assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1));
  tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");

  snprintf(pasta.title, sizeof pasta.title, "Netflix");
  snprintf(pasta.group, sizeof pasta.group, "Streaming");
  fonte(0, "Netflix", "movie");
  fonte(1, "Netflix", "series");
  fonte(2, "Netflix Top 10", "movie");
  fonte(3, "Netflix Top 10", "series");
  fonte(4, "Netflix Kids Top 10", "movie");
  fonte(5, "Netflix Kids Top 10", "series");
  fonte(6, "Latest Netflix", "movie");
  pasta.nSources = 7;
  collection = &pasta;
  snprintf(titulo, sizeof titulo, "%s", pasta.title);
  aberta = 1; anim = 1.0f; source = 0;

  tabFocus = 1; tabCursor = 1;
  snprintf(nome, sizeof nome, "%s-abas-foco.bmp", saida);
  captura(nome, w);

  tabFocus = 0; tabCursor = 0;
  snprintf(nome, sizeof nome, "%s-abas-grade.bmp", saida);
  captura(nome, w);

  // Cursor no fim: a faixa rola para a ultima aba entrar inteira.
  tabFocus = 1; tabCursor = 6;
  snprintf(nome, sizeof nome, "%s-abas-fim.bmp", saida);
  captura(nome, w);

  // PASTA NETFLIX DA CONTA SEM TITULO NAS FONTES (01/10): colecoes.c copia o
  // catId para o titulo e o manifesto nao casou pela base — a aba mostrava
  // "streaming_netflix_movies · Movies". Agora: so o tipo.
  memset(pastaFontes, 0, sizeof pastaFontes);
  fonte(0, "streaming_netflix_movies", "movie");
  fonte(1, "streaming_netflix_series", "series");
  snprintf(pasta.sources[0].catId, sizeof pasta.sources[0].catId, "streaming_netflix_movies");
  snprintf(pasta.sources[1].catId, sizeof pasta.sources[1].catId, "streaming_netflix_series");
  pasta.nSources = 2;
  tabFocus = 1; tabCursor = 0;
  snprintf(nome, sizeof nome, "%s-abas-idcru.bmp", saida);
  captura(nome, w);

  // #359: grade com titulos e o painel da direita, foco na 1a coluna. Com a
  // rail fixa da Moderna a 5a coluna entrava por baixo do painel.
  memset(pastaFontes, 0, sizeof pastaFontes);
  fonte(0, "Oscar", "movie");
  fonte(1, "Oscar", "series");
  pasta.nSources = 2;
  shotGrade = 12; tabFocus = 0; tabCursor = 0; foco = 0;
  snprintf(nome, sizeof nome, "%s-grade-painel.bmp", saida);
  captura(nome, w);
  shotGrade = 0;

  // FILMOGRAFIA DE DIRETOR (205): retrato do pacote, retrato menor ("so TMDB",
  // sem rede aqui) e sem imagem. NV_SHOT_PACOTE aponta o PNG do pacote.
  { const char *pacote = getenv("NV_SHOT_PACOTE");
    const char *fotos[3] = { pacote ? pacote : "deploy/app/art/cinematic/portrait.png",
                             "deploy/app/art/03.jpg", NULL };
    const char *rot[3] = { "pacote", "tmdb", "semfoto" };
    int k;
    // Vistos conhecidos (historico unificado Nuvio/Trakt/Simkl): 1o, 3o e 4o.
    cat_historico_definir_id("tt0000100", "movie", 1);
    cat_historico_definir_id("tt0000102", "movie", 1);
    cat_historico_definir_id("tt0000103", "movie", 1);
    for (k = 0; k < 3; k++) {
      char cmd[1400];
      memset(pastaFontes, 0, sizeof pastaFontes);
      memset(&pasta, 0, sizeof pasta);
      pasta.sources = pastaFontes;
      snprintf(pasta.title, sizeof pasta.title, "Quentin Tarantino");
      snprintf(pasta.group, sizeof pasta.group, "Directors");
      fonte(0, "Quentin Tarantino", "movie");
      pasta.nSources = 1;
      if (fotos[k]) {
        snprintf(pasta.frameDir, sizeof pasta.frameDir, "%s/dir%d", getenv("NUVIO_DADOS"), k);
        snprintf(cmd, sizeof cmd, "mkdir -p '%s' && cp '%s' '%s/portrait%s'", pasta.frameDir, fotos[k],
                 pasta.frameDir, strstr(fotos[k], ".png") ? ".png" : ".jpg");
        assert(system(cmd) == 0);
      }
      snprintf(titulo, sizeof titulo, "%s", pasta.title);
      shotGrade = 8; tabFocus = 0; tabCursor = 0; foco = 0;
      snprintf(nome, sizeof nome, "%s-diretor-%s.bmp", saida, rot[k]);
      captura(nome, w);
      // Mesmo diretor em cada exibicao: o check unico e a barra de progresso.
      if (k == 0) {
        static const int lays[4] = { VT_LAY_GRADE, VT_LAY_LINHA, VT_LAY_RANKING, VT_LAY_SAGA };
        static const char *rl[4] = { "grade", "linha", "ranking", "saga" };
        int q;
        for (q = 0; q < 4; q++) {
          layEscolha = lays[q]; resolverLayout();
          snprintf(nome, sizeof nome, "%s-estilo-%s.bmp", saida, rl[q]);
          captura(nome, w);
        }
        layEscolha = VT_LAY_AUTO; resolverLayout();
      }
    }
    shotGrade = 0;
  }

  tex_encerrar();
  txt_encerrar();
  SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(w);
  SDL_Quit();
  return 0;
}
