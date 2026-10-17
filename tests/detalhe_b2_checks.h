// Real rating renderer, intercepted only at the drawing boundary.
static int b2Panels, b2Tiles;
static GfxRect b2Rects[EX_NFONTES], b2Summary;
static float b2TextBottom;
void b2_text(TxtLinha l, float x, float y, float a) {
  if (a > 0 && x < b2Summary.x + b2Summary.w)
    b2TextBottom = fmaxf(b2TextBottom, y + l.h);
  txt_desenhar_alpha(l, x, y, a);
}
float b2_block(TxtEstilo e, const char *s, int r, int g, int b,
               float x, float y, float w, float lead, float a, int max) {
  float h = txt_bloco_corta(e, s, r, g, b, x, y, w, lead, a, max);
  if (a > 0 && x < b2Summary.x + b2Summary.w)
    b2TextBottom = fmaxf(b2TextBottom, y + h);
  return h;
}
void b2_panel(GfxRect r, float radius, float density, float a) {
  b2Panels++;
  gfx_vidro_painel(r, radius, density, a);
}
void b2_color(GfxRect r, float radius, float cr, float cg, float cb, float a) {
  if (r.w > 400 && r.h >= 448) b2Summary = r;
  gfx_cor(r, radius, cr, cg, cb, a);
}
void b2_tint(GfxRect r, float radius, float x, float y, float size,
             float cr, float cg, float cb, float a) {
  if (b2Tiles < EX_NFONTES) b2Rects[b2Tiles++] = r;
  gfx_luz_canto(r, radius, x, y, size, cr, cg, cb, a);
}
static int b2_checks(void) {
  int fail = 0;
  float season[3], episode[3], gap[3];
  abrir(0, 1, SEC_EPISODIOS, 0);
  for (int p = 0; p < 6; p++) {
    char path[600];
    snprintf(path, sizeof path, "%s/ajustes.txt", dados_dir());
    FILE *f = fopen(path, "w"); assert(f);
    fprintf(f, "detEspacoLocal %d\nescalaTextoLocal %d\n", p % 3, p / 3 ? 5 : 2);
    fclose(f); ajustes_dir(dados_dir()); ajustes_definir_vidro(1);
    gfx_escala_txt_definir(ajustes_escala_texto());
    NotasSecao s = {0};
    for (int i = 0; i < 8; i++) s.cru[i] = 80;
    txt_novo_quadro(); tex_novo_quadro(); gfx_novo_quadro();
    b2Panels = b2Tiles = cardMaterial = 0;
    b2Summary = (GfxRect){0}; b2TextBottom = 0;
    notasui_fontes_desenhar(&s, NV_DETP_X, 100, 1, NULL);
    epPlacaDesenha((GfxRect){100, 100, 640, 414}, 1, 1);
    desenhaEstudio(100, 100, 0, 0, 1);
    int bad = b2TextBottom > b2Summary.y + b2Summary.h - 20.0f;
    printf("%s: summary text bottom %.1f within tile %.1f (scale %d)\n",
           bad ? "FAIL" : "PASS", b2TextBottom, b2Summary.y + b2Summary.h, p / 3);
    fail += bad;
    float castBottom = -14.0f + NV_DETP_EL_AVATAR + NV_DETP_EL_NOME_DY + NV_DETP_EL_PAPEL_DY + 49.0f;
    bad = alturaSecao(SEC_ELENCO) < castBottom;
    printf("%s: cast row %.1f contains tile bottom %.1f\n", bad ? "FAIL" : "PASS", alturaSecao(SEC_ELENCO), castBottom);
    fail += bad;
    bad = b2Panels || cardMaterial;
    printf("%s: B2 tint-only ratings/episodes/studios panels=%d materials=%d\n", bad ? "FAIL" : "PASS", b2Panels, cardMaterial);
    fail += bad;
    bad = b2Tiles != 8 || fabsf(b2Summary.w - 420) > .01f ||
          fabsf(b2Rects[0].y - b2Rects[3].y) > .01f ||
          b2Rects[4].y <= b2Rects[0].y;
    printf("%s: B2 420 summary, eight sources in four columns\n", bad ? "FAIL" : "PASS");
    fail += bad;
    bad = b2Tiles != 8 || fabsf(b2Rects[3].x + b2Rects[3].w - (NV_TELA_W - NV_DETP_X)) > .01f;
    printf("%s: spacing %d ratings right margin\n", bad ? "FAIL" : "PASS", p);
    fail += bad;
    if (p >= 3) { season[p-3] = epTempY(); episode[p-3] = epRowY(); gap[p-3] = progTopoSerie() - epRowY() - epCardH(); }
  }
  int bad = !(season[0] < season[1] && season[1] < season[2] &&
              episode[0] < episode[1] && episode[1] < episode[2] &&
              gap[0] < gap[1] && gap[1] < gap[2]);
  printf("%s: series season/episode anchors and section gaps follow spacing\n", bad ? "FAIL" : "PASS");
  return fail + bad ? 1 : 0;
}
