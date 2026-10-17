// Included by the offline detail-page harness (NUVIO_DETAIL_CHECK=rows).
// Draws the real page while the focus walks each cast/recommendations/
// trailers/collection row and checks the per-frame frost budget.
static int rows_frame(int *capt, int *pass, int *src, int *up) {
  int before = txt_rasterizadas;
  SDL_PumpEvents();
  detail_atualizar(1.0f / 60.0f, SDL_GetTicks());
  txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
  glClear(GL_COLOR_BUFFER_BIT);
  detail_desenhar(SDL_GetTicks());
  if (gfx_vidro_capturas > *capt) *capt = gfx_vidro_capturas;
  if (gfx_vidro_passadas > *pass) *pass = gfx_vidro_passadas;
  if (gfx_vidro_com_fonte > *src) *src = gfx_vidro_com_fonte;
  *up += txt_rasterizadas - before;
  return 0;
}

// Isolate each real renderer: page chrome cannot satisfy a card assertion.
static int card_checks(void) {
  int failures = 0;
  abrir(1, 1, SEC_COLECAO, 0);
  for (int row = 0; row < 4; row++) {
    txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
    glClear(GL_COLOR_BUFFER_BIT);
    cardMaterial = cardFrost = captionSolid = 0;
    foco.fileira = row == 1 ? SEC_RELACIONADOS : SEC_ELENCO;
    foco.coluna = 0;
    if (row == 0) desenhaElenco(100, 100, 0, 1, 1);
    if (row == 1) desenhaRelacionados(100, 100, 1, 0, 1);
    if (row == 2) desenhaTrailer(100, 100, 0, 1, 1);
    if (row == 3) {
      assert(tex_obter_larg(extras_colecao_fundo(), COL_CARD_W));
      desenhaColecao(100, 100, 1, 1);
    }
    int ok = ajustes_vidro() ? cardMaterial == 1 && cardFrost == 1
             : row >= 2 || captionSolid == 1;
    printf("%s: isolated card %d material=%d frost=%d solid=%d\n",
           ok ? "PASS" : "FAIL", row, cardMaterial, cardFrost, captionSolid);
    failures += !ok;
  }
  return failures ? 1 : 0;
}

static int rows_checks(void) {
  static const struct { const char *nome; int sec; int max; } linhas[] = {
    { "cast", SEC_ELENCO, 6 }, { "recommendations", SEC_RELACIONADOS, 7 },
    { "trailers", SEC_TRAILERS, 1 }, { "collection", SEC_COLECAO, 1 },
  };
  int failures = 0;
  for (int serie = 0; serie < 2; serie++)
    for (int l = 0; l < 4; l++) {
      int capt = 0, pass = 0, src = 0, up = 0, cols;
      abrir(serie ? 0 : 1, 1, linhas[l].sec, 0);
      cols = secaoN(linhas[l].sec);
      if (cols < 1) { printf("SKIP: %s %s has no cards\n", serie ? "series" : "movie", linhas[l].nome); continue; }
      for (int frame = 0; frame < 60; frame++) { foco.coluna = frame % cols; rows_frame(&capt, &pass, &src, &up); }
      capt = pass = src = up = 0;
      for (int frame = 0; frame < 180; frame++) {
        foco.coluna = (frame / 6) % cols;
        scrollSec[linhas[l].sec] = (float)(frame % 60) * 6;
        rows_frame(&capt, &pass, &src, &up);
      }
      printf("%s %s: cols=%d frames=180 max-captures=%d max-passes=%d max-frost-panels=%d text-uploads=%d\n",
             serie ? "series" : "movie", linhas[l].nome, cols, capt, pass, src, up);
      if (capt > 1 || pass > 5 || src == 0 || up != 0) {
        fprintf(stderr, "FAIL: %s %s over budget\n", serie ? "series" : "movie", linhas[l].nome);
        failures++;
      }
    }
  printf("%s: per-row frost budget (<=1 capture, <=5 passes, frost panels drawn, no text uploads)\n",
         failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}

// NUVIO_DETAIL_CHECK=mais: the "..." menu panel must be a frosted glass panel,
// not a flat slab. Frost-sampled panels with the menu open must exceed the
// same page with it closed, within the frame budget, and the panel must use
// the card density (GFX_VIDRO_DENS), not the heavier modal one.
// plrui.c is built with gfx_vidro_material renamed to this (detalhe_glass_shot.sh).
static GfxRect spyRect;
static float spyDens;
static int spyN;
void spy_vidro_material(GfxRect r, float raio, float dens, float a) {
  GfxRect cx = detmais_caixa(maisAncora);
  if (fabsf(r.w - cx.w) < 1.0f && fabsf(r.h - cx.h) < 1.0f) { spyRect = r; spyDens = dens; spyN++; }
  gfx_vidro_material(r, raio, dens, a);
}

static int mais_checks(void) {
  int failures = 0;
  for (int serie = 0; serie < 2; serie++) {
    int capt = 0, pass = 0, src_off = 0, src_on = 0, up = 0, d[DMAIS_N];
    abrir(serie ? 0 : 1, 0, 0, 0);
    maisAcoes = 1; botao = nBotoesTodos() - 1;
    for (int k = 0; k < 30; k++) { rows_frame(&capt, &pass, &src_off, &up); SDL_Delay(16); }
    src_off = 0;
    for (int k = 0; k < 10; k++) rows_frame(&capt, &pass, &src_off, &up);
    maisDisponiveis(d);
    detmais_abrir(d);
    for (int k = 0; k < 40; k++) { rows_frame(&capt, &pass, &src_on, &up); SDL_Delay(16); }
    capt = pass = src_on = 0; spyN = 0;
    for (int k = 0; k < 10; k++) rows_frame(&capt, &pass, &src_on, &up);
    printf("%s menu: frost-panels closed=%d open=%d max-captures=%d max-passes=%d panel-calls=%d dens=%.2f (card %.2f)\n",
           serie ? "series" : "movie", src_off, src_on, capt, pass, spyN, spyDens, GFX_VIDRO_DENS);
    if (src_on <= src_off || capt > 1 || pass > 5 || spyN == 0 || spyDens != GFX_VIDRO_DENS) {
      fprintf(stderr, "FAIL: %s menu panel has no frost (or over budget)\n", serie ? "series" : "movie");
      failures++;
    }
    detmais_zerar();
  }
  printf("%s: more-menu panel frosted (open frost panels > closed, <=1 capture, <=5 passes)\n",
         failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
