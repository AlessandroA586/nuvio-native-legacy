// AS EXIBICOES DA PAGINA DE COLECAO (vertudo.c), sem janela e sem rede.
//
// Pedido do dono (06/10): cada colecao escolhe como se mostra — Grade, Linha do
// tempo, Ranking, Saga, Decadas, Mural. O que este teste cobra:
//   1. o Automatico pelo conteudo (Diretores, ranking, franquia, o resto);
//   2. toda exibicao cabe na largura util da grade e nenhuma celula encosta
//      noutra (a tabela de celulas e o unico lugar que posiciona);
//   3. as linhas sao contiguas e crescentes (a navegacao depende disso);
//   4. a ordem cronologica de Saga/Decadas/Linha (sem ano vai para o fim) e os
//      cabecalhos de decada antes dos titulos deles;
//   5. o D-pad: cima/baixo pelo centro mais proximo no Mural, a esquerda no
//      primeiro da linha pede a barra, cima na primeira linha vai ao botao;
//   6. OK no botao troca a exibicao e a escolha fica gravada POR PERFIL.
//
// Inclui src/vertudo.c (o estado e estatico) e troca a lista que a descoberta
// leu por uma semeada aqui.
#define ajustes_ocultar_filmes_vistos shot_hide_watched
#define desc_filme_visto shot_watched
#define desc_vertudo_carregando shot_loading
#define desc_vertudo_fim shot_end
#define desc_vertudo_erro shot_error
#define desc_vertudo_mais shot_more
#define desc_vertudo_filtro_desde shot_filter_from
#define desc_vertudo_pagina shot_page
#define gfx_cor shot_rect
#define gfx_recorte shot_clip
#define gfx_sem_recorte shot_unclip
#define fundo_desenhar_modo shot_background
#define txt_linha_corta shot_text
#define desc_vertudo_n shot_vt_n
#define desc_vertudo_item shot_vt_item
#define desc_vertudo_n_bruto shot_vt_n
#define desc_vertudo_item_bruto shot_vt_item
#include "../src/vertudo.c"
#undef desc_vertudo_n
#undef desc_vertudo_item
#undef desc_vertudo_n_bruto
#undef desc_vertudo_item_bruto
#include <assert.h>

void shot_rect(GfxRect rect, float radius, float r, float g, float b, float a) {
  (void)rect; (void)radius; (void)r; (void)g; (void)b; (void)a;
}
void shot_clip(float x, float y, float w, float h) { (void)x; (void)y; (void)w; (void)h; }
void shot_unclip(void) {}
void shot_background(int mode, GfxRect area, float radius, const char *url, float a) {
  (void)mode; (void)area; (void)radius; (void)url; (void)a;
}
static int hideWatched, allWatched, moreCalls, emptyTexts, exhausted = 1, loading, watchedOlder;
int shot_hide_watched(void) { return hideWatched; }
int shot_watched(const CatItem *it) {
  return !strcmp(it->tipo, "movie") && (allWatched || (watchedOlder && nv_ano_meta(it->meta) < 2020));
}
int shot_loading(void) { return loading; }
int shot_end(void) { return exhausted; }
int shot_error(void) { return 0; }
void shot_more(void) { moreCalls++; loading = 1; }
static int openedSources, resumedPage;
int shot_page(void) { return 300; }
void shot_filter_from(const char *base, const char *type, const char *id,
                      const char *genre, int page) {
  (void)base; (void)type; (void)id; (void)genre;
  openedSources++; resumedPage = page; loading = 1;
}
TxtLinha shot_text(TxtEstilo style, const char *s, int r, int g, int b, int a, float width) {
  (void)style; (void)r; (void)g; (void)b; (void)a; (void)width;
  if (!strcmp(s, i18n("Nenhum título nesta lista."))) emptyTexts++;
  return (TxtLinha){0};
}

static CatItem semente[301];
static int nSemente;
int shot_vt_n(void) { return nSemente; }
int shot_vt_item(int i, CatItem *dst) {
  if (i < 0 || i >= nSemente) return 0;
  if (dst) *dst = semente[i];
  return 1;
}

static const int ANOS[] = { 2005, 1977, 1999, 2015, 1980, 0, 2002, 1983,
                            2017, 2019, 2008, 1997, 2016, 1977 };
#define NANOS (int)(sizeof ANOS / sizeof *ANOS)

static void semear(void) {
  int i;
  nSemente = NANOS;
  for (i = 0; i < nSemente; i++) {
    CatItem *c = &semente[i];
    memset(c, 0, sizeof *c);
    snprintf(c->imdb, sizeof c->imdb, "tt70%05d", i);
    snprintf(c->tipo, sizeof c->tipo, "movie");
    snprintf(c->titulo, sizeof c->titulo, "Capítulo %d", i);
    if (ANOS[i]) snprintf(c->meta, sizeof c->meta, "%d · 2 h", ANOS[i]);
  }
}

static void tecla(SDL_Keycode k) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  vertudo_evento(&e);
}

static int sobrepoe(int a, int b) {
  return celX[a] < celX[b] + celW[b] - 0.5f && celX[b] < celX[a] + celW[a] - 0.5f &&
         celY[a] < celY[b] + celH[b] - 0.5f && celY[b] < celY[a] + celH[a] - 0.5f;
}

static ColFolder pasta;
static ColSource fontes[2];

int main(void) {
  int L, i, j;
  char dir[600];
  snprintf(dir, sizeof dir, "%s/nv-colexib.XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
  assert(mkdtemp(dir));
  setenv("NUVIO_DADOS", dir, 1);
  dados_iniciar("deploy/app/art");
  semear();
  pasta.sources = fontes;
  pasta.nSources = 1;
  snprintf(pasta.id, sizeof pasta.id, "starwars");
  snprintf(pasta.groupId, sizeof pasta.groupId, "franquias");
  snprintf(fontes[0].title, sizeof fontes[0].title, "Star Wars");
  collection = &pasta;
  aberta = 1;

  // 1. AUTOMATICO = exatamente o que a 2.0.3 mostra: Grade para tudo, so o
  // Diretor na Linha do tempo (a 2.0.3 ja fazia). Ranking, Saga, Decadas, Mural
  // e Vitrine existem, mas SO pelo botao "Exibição" (opt-in).
  { static const char *GRUPOS[] = { "Film Collections", "TV Collections", "Awards",
                                    "Studios", "Genres", "" };
    static const char *TITULOS[] = { "Star Wars", "Trilogia do Anel", "Saga Marvel",
                                     "Franquia X", "Top 100", "Franchise" };
    int g, t, ab, r, sv;
    for (g = 0; g < 6; g++) for (t = 0; t < 6; t++)
      for (ab = 1; ab <= 3; ab++) for (r = 0; r < 2; r++) for (sv = 0; sv < 2; sv++) {
        snprintf(pasta.group, sizeof pasta.group, "%s", GRUPOS[g]);
        snprintf(pasta.title, sizeof pasta.title, "%s", TITULOS[t]);
        pasta.nSources = ab; ranked = r; semVitrineAuto = sv;
        assert(layAuto() == VT_LAY_GRADE);
        snprintf(pasta.group, sizeof pasta.group, "Directors");
        assert(layAuto() == VT_LAY_LINHA);
      } }
  ranked = 0; semVitrineAuto = 0; pasta.nSources = 1;
  snprintf(pasta.title, sizeof pasta.title, "Star Wars");
  snprintf(pasta.group, sizeof pasta.group, "Film Collections");
  collection = NULL; assert(layAuto() == VT_LAY_GRADE);
  collection = &pasta;

  // 2-4. GEOMETRIA de cada exibicao
  for (L = VT_LAY_GRADE; L < VT_LAY_VITRINE; L++) {
    lay = L; celLay = -1; orderN = -1; foco = 0;
    montar(nSemente);
    assert(celN == nSemente);
    for (i = 0; i < nSemente; i++) {
      assert(celX[i] >= 0.0f && celY[i] >= 0.0f);
      assert(celX[i] + celW[i] <= VT_CW + 30.0f);   // ranking passa 16 px no maximo
      if (i) assert(celLin[i] >= celLin[i - 1] && celLin[i] <= celLin[i - 1] + 1);
      for (j = 0; j < i; j++) if (sobrepoe(i, j)) {
        fprintf(stderr, "exibicao %d: celulas %d e %d se sobrepoem\n", L, i, j);
        return 1;
      }
    }
    if (ordenaPorAno()) {
      int ant = 0;
      CatItem it;
      for (i = 0; i < nSemente; i++) {
        int y; assert(viewItem(i, &it)); y = yearOf(&it);
        assert(y >= ant); ant = y;
      }
      assert(viewItem(nSemente - 1, &it) && !it.meta[0]);   // sem ano: no fim
    }
    printf("exibicao %-15s ok: %d celulas, %d linhas, altura %.0f\n",
           LAY_NOME[L], celN, celLin[celN - 1] + 1, celFim);
  }
  // Decadas: 1970, 1980, 1990, 2000, 2010 e "sem data" = 6 secoes, na ordem,
  // cada cabecalho acima do primeiro titulo dela.
  lay = VT_LAY_DECADAS; celLay = -1; orderN = -1; montar(nSemente);
  assert(decN == 6);
  assert(decAno[0] == 1970 && decAno[4] == 2010 && decAno[5] == 0);
  assert(decQtd[0] == 2 && decQtd[1] == 2 && decQtd[5] == 1);
  for (i = 1; i < decN; i++) assert(decY[i] > decY[i - 1]);
  assert(celY[0] == decY[0] + VT_DEC_H);

  // 5. D-PAD no Mural: o grande (0) desce para o pequeno de baixo mais perto.
  lay = VT_LAY_MURAL; celLay = -1; orderN = -1; montar(nSemente);
  tabFocus = 0; foco = 0;
  tecla(SDLK_DOWN); assert(foco == 3);           // primeiro pequeno de baixo
  tecla(SDLK_UP);   assert(foco == 1);           // o pequeno de cima, mesmo x
  tecla(SDLK_LEFT); assert(foco == 0);
  pedMenu = 0;
  tecla(SDLK_LEFT); assert(pedMenu == 1 && foco == 0);   // primeiro da linha
  foco = 2; tecla(SDLK_DOWN); assert(foco == 4);
  tecla(SDLK_DOWN); assert(celLin[foco] == 2);   // bloco seguinte, espelhado
  // Cima na primeira linha: a faixa do botao "Exibição".
  foco = 1; tecla(SDLK_UP); assert(tabFocus == 1 && tabCursor == nAbas());

  // 6. OK NO BOTAO troca e grava; outro perfil nao herda.
  salvos_perfil(1);
  layEscolha = VT_LAY_AUTO; lay = VT_LAY_GRADE;
  tecla(SDLK_RETURN);
  assert(lay == VT_LAY_LINHA && layEscolha == VT_LAY_LINHA);
  tecla(SDLK_RETURN); tecla(SDLK_RETURN);
  assert(lay == VT_LAY_SAGA);
  assert(escolhaGravada(&pasta) == VT_LAY_SAGA);
  { ColFolder outra = pasta; snprintf(outra.id, sizeof outra.id, "outra");
    assert(escolhaGravada(&outra) == VT_LAY_AUTO); }
  salvos_perfil(2);
  assert(escolhaGravada(&pasta) == VT_LAY_AUTO);
  gravarEscolha(&pasta, VT_LAY_MURAL);
  assert(escolhaGravada(&pasta) == VT_LAY_MURAL);
  salvos_perfil(1);
  assert(escolhaGravada(&pasta) == VT_LAY_SAGA);
  // Depois do Mural vem a Vitrine, e depois dela volta ao comeco (Grade).
  lay = VT_LAY_MURAL; tabFocus = 1; tabCursor = nAbas();
  tecla(SDLK_RETURN); assert(lay == VT_LAY_VITRINE && tabFocus == 1);
  assert(nAbas() == 0 && tabCursor == 0);
  tecla(SDLK_RETURN); assert(lay == VT_LAY_GRADE);

  // 7. VITRINE: fileiras semeadas direto (a fila de abas precisa de rede).
  pasta.nSources = 2;
  lay = VT_LAY_VITRINE; layEscolha = VT_LAY_VITRINE; tabFocus = 0;
  vitIniciar();
  assert(vitBuf && vitMulti && vitN == 2);
  vitLin[0].brutoN = 5; vitLin[0].estado = VIT_PRONTA;
  vitLin[1].n = 0; vitLin[1].estado = VIT_VAZIA;
  for (i = 0; i < 5; i++) vitBuf[i] = semente[i];
  vitCarga = vitN;
  // A fileira vazia e pulada; cima na primeira vai ao botao; esquerda na
  // primeira coluna pede a barra; direita anda e para na ultima.
  vitFR = 0; vitFC = 0; vitMexeu = 1;
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); assert(vitFC == 2);
  for (i = 0; i < 6; i++) tecla(SDLK_RIGHT);
  assert(vitFC == 4);
  tecla(SDLK_DOWN); assert(vitFR == 0 && vitFC == 4);
  pedMenu = 0; vitFC = 0; tecla(SDLK_LEFT); assert(pedMenu == 1);
  tecla(SDLK_UP); assert(tabFocus == 1 && tabCursor == nAbas());
  tecla(SDLK_DOWN); assert(tabFocus == 0);
  // Segunda fileira com itens: baixo desce lembrando a coluna de cada uma.
  vitLin[1].brutoN = 3; vitLin[1].estado = VIT_PRONTA;
  vitProjetoSujo = 1;
  for (i = 0; i < 3; i++) vitBuf[vitCap + i] = semente[7 + i];
  vitProjetar();
  vitFR = 0; vitFC = 4;
  tecla(SDLK_DOWN); assert(vitFR == 1 && vitFC == 0);   // cada fileira lembra a sua coluna
  tecla(SDLK_RIGHT);
  tecla(SDLK_UP); assert(vitFR == 0 && vitFC == 4);
  // O destaque segue o foco depois do repouso.
  vitAtualizar(0.016f, 1000); vitAtualizar(0.016f, 1300);
  assert(vitHeroAtual == 0 * vitCap + 4);
  tecla(SDLK_RETURN);   // sem catalogo global neste teste: so nao pode cair
  // Sem abas (uma fonte so) as fileiras saem do catalogo: tipos misturados
  // viram Filmes e Series; so filmes, decadas da mais nova a mais velha.
  pasta.nSources = 1; vitIniciar();
  assert(!vitMulti);
  vitAgrupar(nSemente); vitProjetar();
  assert(vitN == 6 && vitLin[0].n >= 1 && strstr(vitLin[0].nome, "2010"));
  assert(!strcmp(vitLin[vitN - 1].nome, i18n("Sem data")));
  snprintf(semente[1].tipo, sizeof semente[1].tipo, "series");
  vitAgrupar(nSemente); vitProjetar();
  assert(vitN == 2 && !strcmp(vitLin[0].nome, i18n("Filmes")) && !strcmp(vitLin[1].nome, i18n("Séries")));

  // Completed rows filtered to zero must draw the empty message.
  pasta.nSources = 2; vitIniciar();
  semear();
  hideWatched = allWatched = exhausted = 1; semFonte = loading = 0;
  vitGuardar(0); vitProjetar();
  assert(vitLin[0].brutoN > 0 && vitLin[0].n == 0);
  exhausted = 1;
  vitDesenhar(1.0f, 100.0f);
  assert(emptyTexts == 1);

  // Drive the real source queue: an all-watched row must not starve row two.
  snprintf(fontes[0].base, sizeof fontes[0].base, "https://unused.invalid/first");
  snprintf(fontes[1].base, sizeof fontes[1].base, "https://unused.invalid/second");
  exhausted = moreCalls = 0;
  vitIniciar();
  vitCarregar(900); assert(openedSources == 1 && loading);
  loading = 0;
  vitCarregar(1000);
  vitCarregar(1010);
  assert(openedSources == 2 && source == 1);
  snprintf(semente[0].tipo, sizeof semente[0].tipo, "series");
  loading = 0;
  vitCarregar(1020); vitProjetar();
  assert(vitLin[1].n == 1 && vitCarga == vitN);
  vitFR = 0; vitFC = 0; tabFocus = 0;
  tecla(SDLK_DOWN); assert(vitFR == 1);
  tecla(SDLK_UP); assert(vitFR == 0);
  tecla(SDLK_RETURN);
  vitCarregar(1025);
  assert(source == 0 && openedSources == 3 && resumedPage == 300);
  loading = 0;
  snprintf(semente[0].imdb, sizeof semente[0].imdb, "tt-resumed");
  vitCarregar(1030); vitProjetar();
  assert(vitLin[0].n == 1 && vitLin[1].n == 1);
  assert(!strcmp(vitItem(0, 0)->imdb, "tt-resumed"));
  hideWatched = 0; vitProjetar();
  assert(vitLin[0].n == NANOS && !strcmp(vitItem(0, 0)->imdb, "tt7000000"));
  hideWatched = 1;

  // A bounded all-watched page must stay available for explicit pagination.
  semear(); vitIniciar();
  semFonte = loading = 0;
  vitCarga = source = 0; vitLin[0].estado = VIT_CARREGANDO;
  vitCarregar(1000);
  assert(vitCarga == 1 && temMaisVistos() && moreCalls == 0);
  tabFocus = 1;
  tecla(SDLK_DOWN); assert(!tabFocus && vitFR == 0);
  tecla(SDLK_RETURN); assert(moreCalls == 1);
  vitCarregar(1070); assert(vitCarga == 0 && moreCalls == 1);
  allWatched = loading = 0;
  vitCarregar(1100); vitProjetar();
  assert(vitCarga == 1 && vitLin[0].n > 0);

  // Single-source grouped rows must expose explicit continuation after 300 watched titles.
  pasta.nSources = 1; vitIniciar();
  semear();
  for (i = 1; i < 301; i++) {
    semente[i] = semente[0];
    snprintf(semente[i].imdb, sizeof semente[i].imdb, "tt-single-%d", i);
  }
  nSemente = 300;
  hideWatched = allWatched = 1; exhausted = loading = moreCalls = 0;
  vitCarregar(1200); vitProjetar();
  assert(vitLin[0].n == 0 && temMaisVistos() && moreCalls == 0);
  tabFocus = 1;
  tecla(SDLK_DOWN); assert(!tabFocus && vitFR == 0);
  tecla(SDLK_RETURN); assert(moreCalls == 1 && loading);
  vitCarregar(1210); assert(moreCalls == 1 && !temMaisVistos());
  snprintf(semente[300].tipo, sizeof semente[300].tipo, "series");
  nSemente = 301; loading = 0; exhausted = 1;
  vitCarregar(1220); vitProjetar();
  assert(vitLin[1].n == 1 && !temMaisVistos());

  // A populated newest decade must not block an empty older decade's button.
  semear();
  for (i = nSemente; i < 301; i++) semente[i] = semente[0];
  snprintf(semente[0].meta, sizeof semente[0].meta, "2025");
  nSemente = 300; allWatched = exhausted = loading = moreCalls = 0; watchedOlder = 1;
  vitIniciar(); loading = 0;
  vitCarregar(1300); vitProjetar();
  assert(vitLin[0].n == 1 && vitLin[1].n == 0 && vitLin[1].mais);
  vitFR = 0; vitFC = tabFocus = 0;
  tecla(SDLK_DOWN); assert(vitFR == 1);
  tecla(SDLK_RETURN); assert(moreCalls == 1 && loading);
  vitCarregar(1310); assert(!temMaisVistos() && moreCalls == 1);
  snprintf(semente[300].meta, sizeof semente[300].meta, "2015");
  watchedOlder = loading = 0; exhausted = 1; nSemente = 301;
  vitCarregar(1320); vitProjetar();
  assert(vitLin[1].n > 0 && !temMaisVistos());

  // An empty non-first source can resume even with watched filtering disabled.
  pasta.nSources = 2; hideWatched = allWatched = exhausted = loading = moreCalls = 0;
  vitIniciar(); vitCarga = source = 1; vitLin[0].estado = VIT_PRONTA;
  vitLin[1].estado = VIT_CARREGANDO; nSemente = 0;
  vitCarregar(4000); vitProjetar();
  assert(vitLin[1].estado == VIT_VAZIA && vitLin[1].mais && temMaisVistos());
  vitFR = 1; vitFC = tabFocus = 0;
  tecla(SDLK_RETURN); assert(moreCalls == 1 && loading);
  semear(); loading = 0;
  vitCarregar(4010); vitProjetar();
  assert(vitLin[1].n == NANOS);

  // Resuming a filtered non-first source preserves its original titles.
  hideWatched = allWatched = 1; moreCalls = 0;
  vitIniciar(); vitCarga = source = 1; vitLin[0].estado = VIT_PRONTA;
  vitLin[1].estado = VIT_CARREGANDO;
  vitCarregar(5000); vitProjetar();
  assert(!vitLin[1].n && vitLin[1].mais);
  source = 0; vitFR = 1; vitFC = tabFocus = 0;
  tecla(SDLK_RETURN); vitCarregar(5010);
  assert(source == 1 && resumedPage == 300 && loading);
  snprintf(semente[0].imdb, sizeof semente[0].imdb, "tt-later-source");
  snprintf(semente[0].tipo, sizeof semente[0].tipo, "series");
  loading = 0; vitCarregar(5020); vitProjetar();
  assert(vitLin[1].n == 1 && !strcmp(vitItem(1, 0)->imdb, "tt-later-source"));
  hideWatched = 0; vitProjetar();
  assert(vitLin[1].n == NANOS && !strcmp(vitItem(1, 0)->imdb, "tt7000000"));
  hideWatched = 1; allWatched = 0; vitProjetoSujo = 1; vitProjetar();
  assert(vitLin[1].n > 0 && !strcmp(vitItem(1, 0)->imdb, "tt7000000"));

  printf("colecao_exibicao: ok\n");
  return 0;
}
