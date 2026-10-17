// Included by the offline detail-page harness (NUVIO_DETAIL_CHECK=rows).
// Draws the real page while the focus walks each cast/recommendations/
// trailers/collection row and checks the per-frame frost budget.
static int rowRects;
static double rowFill, rowBlur;
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
  if (gfx_n_rect > rowRects) rowRects = gfx_n_rect;
  if (gfx_fill > rowFill) rowFill = gfx_fill;
  if (gfx_vidro_fill > rowBlur) rowBlur = gfx_vidro_fill;
  *up += txt_rasterizadas - before;
  return 0;
}

// Isolate each real renderer: page chrome cannot satisfy a card assertion.
static int card_checks(void) {
  int failures = 0;
  for (int serie = 0; serie < 2; serie++) {
  abrir(serie ? 0 : 1, 1, serie ? SEC_ELENCO : SEC_COLECAO, 0);
  for (int row = 0; row < (serie ? 3 : 4); row++) {
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
      // Empty artwork strip above the title/posters must survive the material.
      unsigned char actual[32 * 4], expected[32 * 4];
      glReadPixels(400, 1080 - 120, 32, 1, GL_RGBA, GL_UNSIGNED_BYTE, actual);
      gfx_tex_aspect_atual = tex_aspecto(extras_colecao_fundo());
      gfx_rect((GfxRect){100, 100, COL_CARD_W, COL_CARD_H},
               tex_obter_larg(extras_colecao_fundo(), COL_CARD_W), GFX_VITRINE,
               1, .5f, 0, COL_CARD_RAIO / COL_CARD_H, .30f, 0, 0, 1);
      gfx_tex_aspect_atual = 0;
      glReadPixels(400, 1080 - 120, 32, 1, GL_RGBA, GL_UNSIGNED_BYTE, expected);
      int mismatch = 0;
      for (int i = 0; i < 32 * 4; i++) mismatch += abs(actual[i] - expected[i]) > 1;
      printf("%s: collection artwork pixels mismatch=%d\n", mismatch ? "FAIL" : "PASS", mismatch);
      failures += mismatch != 0;
    }
    int ok = ajustes_vidro() ? cardMaterial == 1 && cardFrost == 1
             : row >= 2 || captionSolid == 1;
    printf("%s: isolated %s card %d material=%d frost=%d solid=%d\n",
           ok ? "PASS" : "FAIL", serie ? "series" : "movie", row, cardMaterial, cardFrost, captionSolid);
    failures += !ok;
  }
  }
  return failures ? 1 : 0;
}

static int rows_checks(void) {
  static const struct { const char *nome; int sec; int max; } linhas[] = {
    { "cast", SEC_ELENCO, 6 }, { "recommendations", SEC_RELACIONADOS, 7 },
    { "trailers", SEC_TRAILERS, 1 }, { "collection", SEC_COLECAO, 1 },
  };
  // Pixel/card checks use the canonical 1080p frost setup separately.
  int failures = getenv("NUVIO_DETAIL_CONFIG") ? 0 : card_checks();
  int frost = ajustes_vidro_fosco() && !gfx_efeitos_minimos();
  for (int serie = 0; serie < 2; serie++)
    for (int l = 0; l < 4; l++) {
      int capt = 0, pass = 0, src = 0, up = 0, cols;
      abrir(serie ? 0 : 1, 1, linhas[l].sec, 0);
      cols = secaoN(linhas[l].sec);
      if (cols < 1) { printf("SKIP: %s %s has no cards\n", serie ? "series" : "movie", linhas[l].nome); continue; }
      for (int frame = 0; frame < 60; frame++) { foco.coluna = frame % cols; rows_frame(&capt, &pass, &src, &up); }
      capt = pass = src = up = 0;
      rowRects = 0; rowFill = rowBlur = 0;
      for (int frame = 0; frame < 180; frame++) {
        foco.coluna = (frame / 6) % cols;
        scrollSec[linhas[l].sec] = (float)(frame % 60) * 6;
        rows_frame(&capt, &pass, &src, &up);
      }
      printf("%s %s: cols=%d frames=180 max-captures=%d max-passes=%d max-frost-panels=%d text-uploads=%d\n",
             serie ? "series" : "movie", linhas[l].nome, cols, capt, pass, src, up);
      printf("%s %s budget: rects=%d fill=%.3f blur-fill=%.3f\n",
             serie ? "series" : "movie", linhas[l].nome, rowRects, rowFill, rowBlur);
      if (capt > 1 || pass > 5 || (frost ? src == 0 : src != 0 || capt != 0 || pass != 0) || up != 0 || rowRects > 260 || rowFill > 5.4 || rowBlur > .75) {
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

// Same patterned backdrop and settled menu; only frost changes. Sample the
// empty left gutter inside the panel, excluding text, icons and focus rows.
static int menu_pixels(int serie) {
  GfxRect r = detmais_caixa(maisAncora);
  unsigned char pixels[2][32 * 4];
  int clearMismatch = 0;
  for (int frost = 0; frost < 2; frost++) {
    setenv("NUVIO_SHOT_VIDRO_FOSCO", frost ? "1" : "0", 1);
    ajustes_teste_vidro_env();
    gfx_novo_quadro(); glClear(GL_COLOR_BUFFER_BIT);
    for (int x = 0; x < 1920; x += 8)
      gfx_cor((GfxRect){ x, 0, 8, 1080 }, 0, (x / 8) % 2 ? 1.0f : .6f, .9f, .8f, 1);
    gfx_vidro_sujar();
    detmais_desenhar(maisAncora, 1);
    glReadPixels((int)r.x + 28, 1080 - (int)r.y - 156, 1, 32, GL_RGBA, GL_UNSIGNED_BYTE, pixels[frost]);
    char id[80];
    snprintf(id, sizeof id, "%s-menu-frost-%d", serie ? "series" : "movie", frost);
    gravar(id);
    if (!frost) {
      unsigned char clearExpected[32 * 4];
      gfx_novo_quadro(); glClear(GL_COLOR_BUFFER_BIT);
      for (int x = 0; x < 1920; x += 8)
        gfx_cor((GfxRect){ x, 0, 8, 1080 }, 0, (x / 8) % 2 ? 1.0f : .6f, .9f, .8f, 1);
      gfx_vidro_sujar();
      plrui_cartao(r, 32, 1);
      glReadPixels((int)r.x + 28, 1080 - (int)r.y - 156, 1, 32, GL_RGBA, GL_UNSIGNED_BYTE, clearExpected);
      for (int i = 0; i < 32 * 4; i++) clearMismatch += abs(clearExpected[i] - pixels[0][i]) > 1;
    }
  }
  int changed = 0, mismatch = 0;
  unsigned char expected[32 * 4];
  gfx_novo_quadro(); glClear(GL_COLOR_BUFFER_BIT);
  for (int x = 0; x < 1920; x += 8)
    gfx_cor((GfxRect){ x, 0, 8, 1080 }, 0, (x / 8) % 2 ? 1.0f : .6f, .9f, .8f, 1);
  gfx_vidro_sujar();
  plrui_cartao(r, 32, 1);
  glReadPixels((int)r.x + 28, 1080 - (int)r.y - 156, 1, 32, GL_RGBA, GL_UNSIGNED_BYTE, expected);
  for (int i = 0; i < 32 * 4; i++) {
    changed += abs(pixels[0][i] - pixels[1][i]) > 2;
    mismatch += abs(expected[i] - pixels[1][i]) > 1;
  }
  printf("%s: menu interior frost pixels changed=%d card-mismatch=%d clear-mismatch=%d\n",
         changed > 16 && mismatch == 0 && clearMismatch == 0 ? "PASS" : "FAIL", changed, mismatch, clearMismatch);
  return changed > 16 && mismatch == 0 && clearMismatch == 0 ? 0 : 1;
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
    failures += menu_pixels(serie);
    detmais_zerar();
  }
  printf("%s: more-menu panel frosted (open frost panels > closed, <=1 capture, <=5 passes)\n",
         failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
