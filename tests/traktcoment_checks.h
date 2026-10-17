// Included by the existing offline detail-page rendering harness.
static int trakt_checks(void) {
  const char *mode = getenv("NUVIO_TRAKT_CHECK");
  int failures = 0;
  if (!strcmp(mode, "vidro")) {
    // Per-frame glass cost of the Trakt row (8 cards, scrolling, focus cycling) vs a normal row
    // (8 studio cards, same focus drive). area = glass-shader px^2; sampled = glass with a backdrop read.
    static const char *rows[2] = { "trakt", "studios" };
    static const char *nome[9] = { "captures", "passes", "fill", "copy", "sampled", "flat", "rects", "uploads", "area" };
    double c[2][9] = {{0}};
    abrir(1, 1, SEC_COMENTARIOS, 0);
    for (int row = 0; row < 2; row++)
      for (int frame = 0; frame < 120; frame++) {
        scrollSec[SEC_COMENTARIOS] = (float)(frame % 90) * 40;
        foco.coluna = (frame / 10) % 8;
        txt_novo_quadro(); gfx_novo_quadro();
        int up = txt_rasterizadas;
        if (!row) desenhaComentarios(96, 300, 1);
        else for (int i = 0; i < 8; i++)
          desenhaEstudio(96 + i * (EST_CARD_W + 20), 300, i % 3, i == foco.coluna ? 1.0f : 0.0f, 1);
        if (frame < 30) continue;
        double v[9] = { gfx_vidro_capturas, gfx_vidro_passadas, gfx_vidro_fill, gfx_vidro_copia,
                        gfx_vidro_com_fonte, gfx_vidro_chapados, gfx_n_rect, txt_rasterizadas - up,
                        gfx_vidro_area };
        for (int k = 0; k < 9; k++) c[row][k] += v[k] / 90.0;
      }
    for (int row = 0; row < 2; row++) {
      printf("%s row per frame:", rows[row]);
      for (int k = 0; k < 9; k++) printf(" %s=%.1f", nome[k], c[row][k]);
      printf("\n");
    }
    // The Trakt row may not put more glass-shader area than a normal row (studios, one 8-card row).
    failures = c[0][8] > c[1][8] * 1.5 + 1 || c[0][4] > c[1][4];
    printf("%s: trakt row glass cost close to a normal row\n", failures ? "FAIL" : "PASS");
    // The governor must not have smoked the glass (frames here are fast, quality Automatica).
    printf("governor: quality=%d degraded=%d\n", ajustes_vidro_qualidade(), gfx_vidro_degradado());
    if (gfx_vidro_degradado()) failures = 1;
  } else if (!strcmp(mode, "series")) {
    // Reopen after choosing episode comments on the previous title.
    comentEp = 1;
    abrir(0, 1, SEC_COMENTARIOS, 2);
    if (comentEp != 0 || nCartoesCom() != 3 || secaoN(SEC_COMENTARIOS) != 5) {
      fprintf(stderr, "FAIL: series must reopen on its three show comments (mode=%d cards=%d)\n",
              comentEp, nCartoesCom());
      failures++;
    }
    // Moving across the episode pill must not select it.
    foco.coluna = 1; quadros(1);
    if (comentEp) failures++;
    // OK still selects episode comments, even when that episode has none.
    SDL_Event e; memset(&e, 0, sizeof e);
    e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
    detail_evento(&e);
    e.type = SDL_KEYUP; detail_evento(&e);
    if (comentEp != 1 || secaoN(SEC_COMENTARIOS) != 2) {
      fprintf(stderr, "FAIL: episode pill mode=%d columns=%d\n", comentEp, secaoN(SEC_COMENTARIOS));
      failures++;
    }
    abrir(1, 1, SEC_COMENTARIOS, 0);
    abrir(0, 1, SEC_COMENTARIOS, 2);
    if (comentEp || nCartoesCom() != 3) failures++;
    printf("%s: series default, pill traversal, episode selection, movie/series reopen\n",
           failures ? "FAIL" : "PASS");
  } else {
    abrir(1, 1, SEC_COMENTARIOS, 0);
    // Draw the real row while scrolling, after warming every card and focus color.
    for (int frame = 0; frame < 180; frame++) {
      scrollSec[SEC_COMENTARIOS] = (float)(frame % 90) * 40;
      foco.coluna = (frame / 10) % 8;
      txt_novo_quadro(); gfx_novo_quadro();
      desenhaComentarios(96, 300, 1);
    }
    traktLayoutCalls = 0;
    int uploads = 0, sampled = 0;
    for (int frame = 0; frame < 180; frame++) {
      scrollSec[SEC_COMENTARIOS] = (float)(frame % 90) * 40;
      foco.coluna = (frame / 10) % 8;
      txt_novo_quadro(); gfx_novo_quadro();
      int before = txt_rasterizadas;
      desenhaComentarios(96, 300, 1);
      uploads += txt_rasterizadas - before;
      sampled += gfx_vidro_com_fonte;
    }
    printf("comments scrolling: frames=180 layout-calls=%d text-uploads=%d frost-sampled-cards=%d\n",
           traktLayoutCalls, uploads, sampled);
    failures = traktLayoutCalls != 0 || uploads != 0 || sampled == 0;
    printf("%s: no warmed comment wrapping or text uploads per frame\n", failures ? "FAIL" : "PASS");
  }
  return failures ? 1 : 0;
}
