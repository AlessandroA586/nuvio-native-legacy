// Included by the existing offline detail-page rendering harness.
static int trakt_checks(void) {
  const char *mode = getenv("NUVIO_TRAKT_CHECK");
  int failures = 0;
  if (!strcmp(mode, "series")) {
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
