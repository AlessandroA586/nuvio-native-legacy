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
