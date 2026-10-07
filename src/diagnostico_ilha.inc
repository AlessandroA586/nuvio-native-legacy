// DIAGNOSTICO, TESTE DE VELOCIDADE E APRESENTACAO NO GLASS UI (mockup de
// Ajustes, quadros "diagnostico*" e "velocidade", aprovado em 03/10). As
// telas viram ilhas sobre a arte, com as pecas de Ajustes (ajustes_ui_*). Os
// numeros sao os mesmos de antes; so o desenho mudou. Incluido em
// diagnostico.c antes de diagnostico_desenhar.

#define DG_TX 243, 242, 239, 255
#define DG_VERDE 0.298f, 0.765f, 0.541f
#define DG_AMBAR 0.910f, 0.722f, 0.290f
#define DG_VERM  0.898f, 0.325f, 0.294f
#ifdef AJUSTES_TESTE
static int dgMemFixa;
#endif
static float dgCapsCom(TxtEstilo e, const char *s, float track, float x, float y, int r, int g, int b, float a) {
  char up[200];
  idioma_maiusc_em(ajustes_idioma(), up, sizeof up, i18n(s));
  return txt_tracking(e, up, r, g, b, x, y, a, track);
}
static float ajCapsDg(const char *s, float x, float y) { return dgCapsCom(TXT_AJ_CAPS13, s, 1.82f, x, y, 243, 242, 239, 0.45f); }
static float ajCapsCor(const char *s, float x, float y, float r, float g, float b) {
  return dgCapsCom(TXT_MINI, s, 2.1f, x, y, (int)(r * 255), (int)(g * 255), (int)(b * 255), 1);
}
static float dgX0(void) { return 48.0f + ajustes_rail_largura_fixa(); }

static TxtLinha dgT(TxtEstilo e, const char *s) { return txt_linha(e, s, DG_TX); }
static TxtLinha dgTC(TxtEstilo e, const char *s, float w) { return txt_linha_corta(e, s, DG_TX, w); }
static float dgBloco(TxtEstilo e, const char *s, float x, float y, float w, float lh, float al, int n) {
  return txt_bloco(e, i18n(s), 243, 242, 239, x, y, w, lh, al, n);
}
static void dgIcone(const char *n, float x, float y, float l, float al) {
  gfx_icone((GfxRect){ x, y, l, l }, n, 0.953f, 0.949f, 0.937f, al);
}
// Titulo e sub da tela ao lado da ilha do relogio (o cabDiag do mockup).
static void dgCabecalho(const char *titulo, const char *sub) {
  float x = ajustes_ilha_x() + 202.0f;
  TxtLinha t = dgT(TXT_AJ_TIT28, titulo), s = dgTC(TXT_AJ_ESTADO, sub, NV_TELA_W - x - t.w - 60);
  txt_desenhar(t, x, 64 - t.h * 0.5f);
  txt_desenhar_alpha(s, x + t.w + 18, 64 - s.h * 0.5f, 0.62f);
}
// Kicker + titulo de ilha.
static float dgTitulo(const char *kick, const char *titulo, TxtEstilo e, float x, float y, float w) {
  ajustes_ui_kicker(kick, x, y, 1);
  y += 22.0f;
  return 22.0f + dgBloco(e, titulo, x, y, w, e == TXT_ILHA_TITULO ? 46.0f : 40.0f, 1, 2);
}
// Barra com o brilho que corre DENTRO do preenchido (pedido de 26/09).
static void dgBarra(GfxRect r, float pct, int animado, Uint32 agora) {
  float ar, ag, ab, w = r.w * limitePct(pct) / 100.0f;
  ajustes_acento(&ar, &ag, &ab);
  gfx_cor(r, 0.5f, 1, 1, 1, 0.08f);
  if (w > 1.0f) gfx_cor((GfxRect){ r.x, r.y, w, r.h }, 0.5f, ar, ag, ab, 1);
  if (animado && w > 24.0f && !ajustes_animacoes_reduzidas()) {
    float larg = w < 240.0f ? w * 0.35f : 84.0f;
    float x0 = r.x - larg + fmodf((float)agora * 0.24f, w + larg), x1 = x0 + larg;
    if (x0 < r.x) x0 = r.x;
    if (x1 > r.x + w) x1 = r.x + w;
    if (x1 - x0 > 1.0f) gfx_cor((GfxRect){ x0, r.y, x1 - x0, r.h }, 0.5f, 1, 1, 1, 0.22f);
  }
}
static void dgLinha(float x0, float y0, float x1, float y1, float esp, float r, float g, float b, float a) {
  float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
  int n, i;
  if (len < 1.0f) return;
  n = (int)ceilf(len / 40.0f);
  for (i = 0; i < n; i++) {
    float ax = x0 + dx * i / n, ay = y0 + dy * i / n, bx = x0 + dx * (i + 1) / n, by = y0 + dy * (i + 1) / n;
    float halo = 2.0f;
    GfxRect q = { fminf(ax, bx) - halo, fminf(ay, by) - halo, fabsf(bx - ax) + 2 * halo, fabsf(by - ay) + 2 * halo };
    gfx_rect(q, 0, GFX_LINHA, esp / q.h, ((bx - ax) * (by - ay) < 0.0f) ? 1.0f : 0.0f, 0, halo / q.h, r, g, b, a);
  }
}
// "Rotulo ...... valor" com fio de 7% (.stat do mockup).
static float dgStat(const char *k, const char *v, float x, float y, float w) {
  return ajustes_ui_meta(i18n(k), v, x, y, w) - 4.0f;
}
static void dgPonto(float x, float yc, float r, float g, float b) {
  gfx_cor((GfxRect){ x, yc - 4.5f, 9, 9 }, 0.5f, r, g, b, 1);
}

// --- APRESENTACAO (primeira abertura e o "Antes de comecar") ---------------------
static void dgApresentacao(int primeiraAbertura) {
  GfxRect m = { 300, 0, 1320, 0 };
  float x, y, cw;
  static const char *const IC[3] = { "aj_images", "aj_activity", "aj_circle-play" };
  static const char *const TX[3] = { "Artes, logos e fundos", "Tempos, bytes e falhas", "Fontes de vídeo" };
  int i;
  m.h = 52 + 22 + 48 + 22 + 25 + 14 + 150 + 28 + 60 + 18 + 56 + 30 + 30 + 52;
  m.y = (NV_TELA_H - m.h) * 0.5f;
  ajustes_ui_veu();
  ajustes_ui_ilha(m, 36, 1);
  x = m.x + 56; y = m.y + 52;
  ajustes_ui_kicker("Diagnóstico e otimização", x, y, 1);
  y += 22;
  { TxtLinha t = dgT(TXT_ILHA_TITULO, "Antes de começar"); txt_desenhar(t, x, y); y += 48; }
  y += 22;
  { TxtLinha t = dgT(TXT_AJ_SUB, "O que será medido"); txt_desenhar_alpha(t, x, y, 0.55f); y += 25 + 14; }
  cw = (m.w - 112 - 32) / 3.0f;
  for (i = 0; i < 3; i++) {
    GfxRect c = { x + i * (cw + 16), y, cw, 150 };
    TxtLinha t = dgT(TXT_ILHA_ITEM, TX[i]);
    ajustes_ui_neutro(c, 24, 0.06f);
    ajustes_ui_neutro((GfxRect){ c.x + 24, c.y + 24, 52, 52 }, 16, 0.08f);
    dgIcone(IC[i], c.x + 24 + 13, c.y + 24 + 13, 26, 0.85f);
    txt_desenhar(t, c.x + 24, c.y + 150 - 24 - t.h);
  }
  y += 150 + 28;
  y += dgBloco(TXT_AJ_TEXTO, "O app lê manifestos e catálogos, testa artes e fontes de vídeo e compara o caminho de carregamento desta TV.",
               x, y, m.w - 112, 30, 0.62f, 2) + 18;
  y += ajustes_ui_ef("aj_triangle-alert", i18n("Haverá testes de rede e consumo de dados. O relatório técnico será enviado automaticamente ao suporte."),
                     x, y, m.w - 112);
  y += 30;
  { TxtLinha n = dgT(TXT_ILHA_GENERO, "Não altera assistidos, histórico, progresso ou scrobbling.");
    const char *k[2] = { "OK", "Voltar" };
    const char *l[2] = { primeiraAbertura ? "Continuar" : "Começar", primeiraAbertura ? "Fechar" : "Cancelar" };
    float dw = ajustes_ui_dicas(k, l, 2, 0, 0, 0);
    txt_desenhar_alpha(n, x, y + (30 - n.h) * 0.5f, 0.48f);
    ajustes_ui_dicas(k, l, 2, m.x + m.w - 56 - dw, y, 1); }
}

// --- ESTADO 0: OBJETIVO --------------------------------------------------------
static void dgObjetivo(void) {
  GfxRect e = { dgX0(), 112, 900, NV_TELA_H - 112 - 40 };
  GfxRect dr = { e.x + e.w + 24, 40, 0, NV_TELA_H - 80 };
  float x = e.x + 44, w = e.w - 88, y, ar, ag, ab;
  PtvPerfil atual, cand;
  int travado, i;
  long mem;
  static const char *const PASSOS[5] = {
    "Manifestos, catálogos e fontes de vídeo dos add-ons",
    "Cada fonte de arte: catálogo, Metahub, TMDB, Trakt, Apple TV, fanart.tv, anime e logo",
    "As mesmas artes pelo cache, com o perfil atual",
    "Aplica o candidato e mede as mesmas artes de novo",
    "Aplica só se ajudar; senão restaura o perfil anterior",
  };
  static const char *const PIC[5] = { "aj_puzzle", "aj_images", "aj_database", "aj_zap", "aj_rotate-ccw-clock" };
  dr.w = NV_TELA_W - 40 - dr.x;
  ajustes_acento(&ar, &ag, &ab);
  perfilAtual(&atual, &travado, &mem);
  ptv_candidato(ptv_plataforma(), mem, focoModo ? PTV_DESEMPENHO : PTV_QUALIDADE, travado, &cand);
  ajustes_ui_ilha(e, 36, 0);
  y = e.y + 42;
  y += dgTitulo("Desempenho desta TV", "Diagnóstico e otimização", TXT_ILHA_TITULO, x, y, w);
  y += 8 + dgBloco(TXT_AJ_SUB, "Teste os addons, artes e fontes desta TV; o relatório é enviado ao suporte.", x, y + 8, w, 26.6f, 0.55f, 2);
  y += 34;
  { TxtLinha t = dgT(TXT_AJ_SECAO, "Objetivo"), d2 = dgT(TXT_ILHA_GENERO, "Esquerda e direita escolhem o objetivo");
    txt_desenhar(t, x, y); txt_desenhar_alpha(d2, x + w - d2.w, y + t.h - d2.h - 2, 0.42f); y += t.h + 16; }
  for (i = 0; i < 2; i++) {
    float cw = (w - 16) * 0.5f;
    GfxRect c = { x + i * (cw + 16), y, cw, 126 };
    int sel = focoModo == i;
    if (sel && !focoLinha) ajustes_ui_foco_linha(c, 26);
    else ajustes_ui_neutro(c, 26, sel ? 0.14f : 0.05f);
    { TxtLinha t = dgT(TXT_AJ_NOME, i ? "Desempenho" : "Qualidade");
      float tx = c.x + 26;
      if (sel) { gfx_cor((GfxRect){ tx, c.y + 26 + t.h * 0.5f - 4, 8, 8 }, 0.5f, ar, ag, ab, 1); tx += 18; }
      txt_desenhar_alpha(t, tx, c.y + 26, sel ? 1.0f : 0.8f);
      dgBloco(TXT_AJ_ESTADO, i ? "Fontes mais leves, menos antecipação e menor pressão de memória"
                               : "Nitidez na resolução exibida e preferências atuais preservadas",
              c.x + 26, c.y + 26 + t.h + 10, c.w - 52, 24.6f, 0.58f, 2); }
  }
  y += 126 + 32;
  { TxtLinha t = dgT(TXT_AJ_SECAO, "Candidato desta rodada"); txt_desenhar(t, x, y); y += t.h + 6; }
  y += ajustes_ui_antes_depois("Memória para imagens", atual.texMb, cand.texMb, "MB", 512, x, y, w, 0, -1, 0, 0);
  y += ajustes_ui_antes_depois("Fios de rede das artes", atual.fiosRede, cand.fiosRede, "fios", 6, x, y, w, 0, -1, 0, 0);
  y += ajustes_ui_antes_depois("Largura do fundo em tela cheia", atual.heroiLarg, cand.heroiLarg, "px", 2560, x, y, w, 0, -1, 0, 0);
  y += 14;
  y += dgBloco(TXT_ILHA_GENERO, "Se o reteste da mesma amostra piorar, o perfil anterior volta sozinho e a tela diz o motivo.", x, y, w, 24, 0.48f, 2);
  if (travado) {
    TxtLinha t = txt_linha_corta(TXT_ILHA_GENERO, i18n("A memória para imagens foi escolhida em Ajustes e não muda."), 232, 184, 74, 255, w);
    txt_desenhar(t, x, y + 8);
  }
  { float by = e.y + e.h - 42 - 18 - 16 - 60, bx = x;
    bx += ajustes_ui_botao(i18n("Iniciar diagnóstico"), "aj_activity", bx, by, !focoLinha) + 12;
    ajustes_ui_botao(i18n("Teste de velocidade"), "aj_gauge", bx, by, focoLinha);
    { TxtLinha t = dgTC(TXT_ILHA_APOIO, i18n("Até 3 títulos, 6 fontes e 12 imagens; limite de 8 minutos"), w);
      txt_desenhar_alpha(t, x, by + 60 + 16, 0.45f); } }
  // Como funciona
  ajustes_ui_ilha(dr, 36, 0);
  x = dr.x + 44; w = dr.w - 88; y = dr.y + 42;
  ajustes_ui_kicker("Como funciona", x, y, 1);
  { TxtLinha t = dgT(TXT_AJ_TIT28, "Cinco etapas, nesta ordem"); txt_desenhar(t, x, y + 22 + 6); y += 22 + 6 + t.h + 30; }
  gfx_cor((GfxRect){ x + 27, y + 30, 2, 4 * 104.0f }, 0, 1, 1, 1, 0.08f);
  for (i = 0; i < 5; i++) {
    float cy = y + 52 + i * 104.0f, h;
    char k[32];
    gfx_cor((GfxRect){ x, cy - 28, 56, 56 }, 0.5f, 0.106f, 0.110f, 0.129f, 1);
    gfx_anel((GfxRect){ x, cy - 28, 56, 56 }, 0.5f, 1, 1, 1, 1, 0.08f);
    dgIcone(PIC[i], x + 16, cy - 12, 24, 0.8f);
    snprintf(k, sizeof k, i18n("Etapa %d"), i + 1);
    h = txt_bloco(TXT_AJ_ITEM, i18n(PASSOS[i]), 243, 242, 239, x + 78, -4000, w - 78, 29.4f, 0, 2);
    ajCapsDg(k, x + 78, cy - (18 + 4 + h) * 0.5f);
    txt_bloco(TXT_AJ_ITEM, i18n(PASSOS[i]), 243, 242, 239, x + 78, cy - (18 + 4 + h) * 0.5f + 22, w - 78, 29.4f, 0.88f, 2);
  }
  { float h2 = 0, yb = dr.y + dr.h - 42;
    float a1 = txt_bloco(TXT_AJ_ESTADO, i18n("O relatório compacto é guardado localmente e só fica como enviado após confirmação do servidor."), 0, 0, 0, 0, -4000, w - 64, 24.65f, 0, 3);
    h2 = 28 + 24.65f;
    yb -= h2;
    ajustes_ui_ef("aj_eye", i18n("Não altera assistidos, histórico, progresso ou scrobbling."), x, yb, w);
    yb -= 10 + a1 + 28;
    ajustes_ui_ef("aj_file-text", i18n("O relatório compacto é guardado localmente e só fica como enviado após confirmação do servidor."), x, yb, w); }
}

// --- ESTADO 1: EM ANDAMENTO ----------------------------------------------------
static void dgAndamento(Uint32 agora) {
  static const char *const ETAPAS[6] = {
    "Sondando manifestos, catálogos e fontes", "Medindo cada fonte de arte", "Artes com o cache frio",
    "Artes com o perfil atual", "Artes com o perfil candidato", "Gravando e enviando o relatório",
  };
  int fase = atomic_load(&d.fase), feito = atomic_load(&d.feitos), total = atomic_load(&d.total), i;
  float pct, ar, ag, ab;
  GfxRect e = { dgX0(), 112, 1100, NV_TELA_H - 152 }, dr;
  float x = e.x + 48, w = e.w - 96, y;
  char v[96];
  ajustes_acento(&ar, &ag, &ab);
  if (fase < 1) fase = 1;
  if (fase > 6) fase = 6;
  pct = fase <= 2 ? (total > 0 ? 60.0f * (float)feito / (float)total : 4.0f) : 60.0f + (float)(fase - 2) * 9.0f;
  dgCabecalho(i18n("Diagnóstico e otimização"), i18n("Teste os addons, artes e fontes desta TV"));
  ajustes_ui_ilha(e, 36, 0);
  y = e.y + 44;
  { char k[160];
    snprintf(k, sizeof k, "%s · %s", i18n("Diagnóstico em andamento"), i18n(d.modo == DIAG_DESEMPENHO ? "Desempenho" : "Qualidade"));
    ajustes_ui_kicker(k, x, y, 1); }
  { TxtLinha t = dgT(TXT_AJ_INSP, ETAPAS[fase - 1]); txt_desenhar(t, x, y + 26); y += 26 + t.h + 22; }
  snprintf(v, sizeof v, "%d%%", (int)pct);
  { TxtLinha n = dgT(TXT_AJ_NUM110, v), s;
    char es[64];
    snprintf(es, sizeof es, i18n("etapa %d de %d"), fase, 6);
    s = dgT(TXT_AJ_SUB, es);
    txt_desenhar(n, x, y);
    txt_desenhar_alpha(s, x + n.w + 14, y + n.h * 0.78f - s.h, 0.5f);
    y += n.h * 0.9f + 24; }
  dgBarra((GfxRect){ x, y, w, 14 }, pct, 1, agora);
  y += 14 + 34;
  for (i = 0; i < 6; i++) {
    int feita = i + 1 < fase, at = i + 1 == fase;
    GfxRect r = { x, y + i * 64, w, 58 };
    TxtLinha t = txt_linha(at ? TXT_ILHA_ITEM : TXT_AJ_ITEM, ETAPAS[i], DG_TX);
    float cx = r.x + 18 + 15, cy = r.y + 29;
    if (at) ajustes_ui_neutro(r, 20, 0.08f);
    if (feita) { gfx_cor((GfxRect){ cx - 15, cy - 15, 30, 30 }, 0.5f, ar, ag, ab, 0.22f);
                 gfx_icone((GfxRect){ cx - 8, cy - 8, 16, 16 }, "aj_check", ar, ag, ab, 1); }
    else if (at) { float p = ajustes_animacoes_reduzidas() ? 1.0f : 0.55f + 0.45f * sinf((float)agora * 0.0052f);
                   gfx_cor((GfxRect){ cx - 12, cy - 12, 24, 24 }, 0.5f, ar, ag, ab, 0.22f * p);
                   gfx_cor((GfxRect){ cx - 6, cy - 6, 12, 12 }, 0.5f, ar, ag, ab, 1); }
    else { char n[4]; TxtLinha l; snprintf(n, sizeof n, "%d", i + 1); l = dgT(TXT_AJ_KBD, n);
           gfx_cor((GfxRect){ cx - 15, cy - 15, 30, 30 }, 0.5f, 1, 1, 1, 0.06f);
           txt_desenhar_alpha(l, cx - l.w * 0.5f, cy - l.h * 0.5f, 0.4f); }
    txt_desenhar_alpha(t, r.x + 66, cy - t.h * 0.5f, feita ? 0.62f : at ? 1.0f : 0.4f);
    if (at) { TxtLinha a = dgT(TXT_ILHA_GENERO, "agora"); txt_desenhar_alpha(a, r.x + r.w - 18 - a.w, cy - a.h * 0.5f, 0.5f); }
  }
  { const char *k[1] = { "Voltar" }, *l[1] = { "Cancelar" };
    float by = e.y + e.h - 44 - 30, dw = ajustes_ui_dicas(k, l, 1, 0, 0, 0);
    TxtLinha t = dgT(TXT_ILHA_GENERO, "Voltar cancela e interrompe as requisições da sessão.");
    txt_desenhar_alpha(t, x, by + (30 - t.h) * 0.5f, 0.48f);
    ajustes_ui_dicas(k, l, 1, x + w - dw, by, 1); }
  // AGORA NESTA TV
  dr = (GfxRect){ e.x + e.w + 24, 112, 0, NV_TELA_H - 152 };
  dr.w = NV_TELA_W - 40 - dr.x;
  ajustes_ui_ilha(dr, 36, 0);
  x = dr.x + 40; w = dr.w - 80; y = dr.y + 40;
  ajustes_ui_kicker("Agora nesta TV", x, y, 1);
  y += 18 + 14;
  { long bytes = 0, lim = tex_orcamento_bytes();
    char a[32], b[96];
    TxtLinha n, s;
    tex_estatisticas(NULL, NULL, &bytes, NULL, NULL);
#ifdef AJUSTES_TESTE
    if (dgMemFixa) { bytes = (long)(151.4 * 1048576); lim = 240L << 20; }
#endif
    snprintf(a, sizeof a, "%ld", bytes >> 20);
    snprintf(b, sizeof b, i18n("/ %ld MB de imagens"), lim >> 20);
    n = dgT(TXT_ILHA_TITULO, a); s = dgT(TXT_AJ_TEXTO, b);
    txt_desenhar(n, x, y);
    txt_desenhar_alpha(s, x + n.w + 10, y + n.h * 0.8f - s.h, 0.55f);
    y += n.h + 16;
    ajustes_ui_grafico_memoria(x, y, w, 150);
    y += 150 + 8;
    { TxtLinha l = dgT(TXT_AJ_MINI14, "há 2 min"), r2 = dgT(TXT_AJ_MINI14, "agora");
      txt_desenhar_alpha(l, x, y, 0.42f); txt_desenhar_alpha(r2, x + w - r2.w, y, 0.42f); y += 17 + 22; }
    snprintf(b, sizeof b, i18n("%d de %d"), feito, total);
    y += dgStat("Etapas de rede", b, x, y, w);
    snprintf(b, sizeof b, "%ld / %ld MB", bytes >> 20, lim >> 20);
    y += dgStat("Memória usada por imagens", b, x, y, w);
    { int fios = 0, fiosMax = 0;
      tex_threads_info(&fios, &fiosMax);
      snprintf(b, sizeof b, i18n("%d de %d"), fios, fiosMax);
      { TxtLinha vl = dgT(TXT_ILHA_GENERO, b); int k;
        float ar2, ag2, ab2;
        ajustes_acento(&ar2, &ag2, &ab2);
        for (k = 0; k < fiosMax && k < 12; k++) {
          float px = x + w - vl.w - 10 - (fiosMax - k) * 15.0f;
          if (k < fios) gfx_cor((GfxRect){ px, y + 13 + (vl.h - 10) * 0.5f, 10, 10 }, 0.3f, ar2, ag2, ab2, 1);
          else gfx_cor((GfxRect){ px, y + 13 + (vl.h - 10) * 0.5f, 10, 10 }, 0.3f, 1, 1, 1, 0.12f);
        } }
      y += dgStat("Fios em uso", b, x, y, w); }
    snprintf(b, sizeof b, "%d px", tex_teto_heroi());
    y += dgStat("Largura do fundo em tela cheia", b, x, y, w); }
  if (d.perfCand.texMb > 0) {
    y += 28;
    ajustes_ui_kicker("Candidato desta rodada", x, y, 1);
    y += 18 + 8;
    y += ajustes_ui_antes_depois("Memória para imagens", d.perfAntes.texMb, d.perfCand.texMb, "MB", 512, x, y, w, 0, -1, 0, 0);
    y += ajustes_ui_antes_depois("Fios de rede das artes", d.perfAntes.fiosRede, d.perfCand.fiosRede, "fios", 6, x, y, w, 0, -1, 0, 0);
    ajustes_ui_antes_depois("Largura do fundo em tela cheia", d.perfAntes.heroiLarg, d.perfCand.heroiLarg, "px", 2560, x, y, w, 0, -1, 0, 0);
  }
}

// --- ESTADO 2: RESULTADO --------------------------------------------------------
static void dgResultado(void) {
  float X = dgX0(), W = NV_TELA_W - 40 - X, cw = (W - 18) * 0.5f, ar, ag, ab;
  GfxRect tl = { X, 112, cw, 452 }, tr = { X + cw + 18, 112, cw, 452 };
  GfxRect bl = { X, 112 + 452 + 18, cw, NV_TELA_H - 40 - 60 - 18 - (112 + 452 + 18) }, br = { tr.x, bl.y, cw, bl.h };
  int cobertura = coberturaPercentual(), cor, i;
  char v[200];
  float x, y, w;
  ajustes_acento(&ar, &ag, &ab);
  dgCabecalho(i18n("Diagnóstico e otimização"), i18n(d.coberturaParcial ? "Cobertura parcial da amostra" : "Amostra concluída"));
  // Resultado geral
  ajustes_ui_ilha(tl, 32, 0);
  x = tl.x + 34; w = tl.w - 68; y = tl.y + 30;
  ajustes_ui_kicker("Resultado geral", x, y, 1);
  y += 28;
  snprintf(v, sizeof v, "%d%%", cobertura);
  { TxtLinha n = dgT(TXT_AJ_NUM64, v), s = dgT(TXT_AJ_SUB, "cobertura da amostra");
    txt_desenhar(n, x, y); txt_desenhar_alpha(s, x + n.w + 12, y + n.h * 0.8f - s.h, 0.5f); y += n.h * 0.92f + 18; }
  dgBarra((GfxRect){ x, y, w, 10 }, (float)cobertura, 0, 0);
  y += 10 + 20;
  { TxtLinha k = dgT(TXT_AJ_ESTADO, "Gargalo principal"), g = txt_linha(TXT_AJ_ESTADO, gargaloPrincipal(), (int)(0.910f * 255), (int)(0.722f * 255), (int)(0.290f * 255), 255);
    gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
    txt_desenhar_alpha(k, x, y + 13, 0.45f); txt_desenhar(g, x + w - g.w, y + 13); y += 42.5f; }
  { int ok = d.imagensOk, fal = d.imagensFalhas, k;
    for (k = 1; k < PTV_N_FONTES; k++) { ok += d.fonte[k].ok; fal += d.fonte[k].falhas; }
    snprintf(v, sizeof v, i18n("%d ok · %d falhas"), ok, fal);
    y += dgStat("Artes", v, x, y, w); }
  { const char *rel = atomic_load(&d.enviando) ? "enviando…" : d.envioFalhou ? "guardado; envio falhou"
                    : d.enviado ? "enviado ao suporte" : "guardado localmente";
    TxtLinha k = dgT(TXT_AJ_ESTADO, "Relatório");
    TxtLinha r = d.envioFalhou ? txt_linha(TXT_AJ_ESTADO, rel, 229, 83, 75, 255) : d.enviado ? txt_linha(TXT_AJ_ESTADO, rel, 76, 195, 138, 255) : dgT(TXT_AJ_ESTADO, rel);
    gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
    txt_desenhar_alpha(k, x, y + 13, 0.45f);
    txt_desenhar(r, x + w - r.w, y + 13);
    if (d.envioFalhou) dgPonto(x + w - r.w - 17, y + 13 + r.h * 0.5f, DG_VERM);
    else if (d.enviado) dgPonto(x + w - r.w - 17, y + 13 + r.h * 0.5f, DG_VERDE); }
  // Artes por fonte
  ajustes_ui_ilha(tr, 32, 0);
  x = tr.x + 34; w = tr.w - 68; y = tr.y + 30;
  ajustes_ui_kicker("Artes por fonte", x, y, 1);
  { TxtLinha s = dgT(TXT_ILHA_APOIO, "Tempo médio por arte: consulta e download"); txt_desenhar_alpha(s, x, y + 24, 0.45f); }
  y += 24 + 20 + 16;
  { int f, maior = 1, lenta = 0, msLenta = -1, linhas = 0;
    float passo;
    for (f = 1; f < PTV_N_FONTES; f++) {
      int ms = ptv_fonte_ms(&d.fonte[f]);
      if (ms > maior) maior = ms;
      if (ms > msLenta) { msLenta = ms; lenta = f; }
      if (d.fonte[f].ok || d.fonte[f].falhas) linhas++;
    }
    passo = linhas > 8 ? 248.0f / linhas : 31.0f;
    for (f = 1; f < PTV_N_FONTES; f++) {
      const PtvFonte *s = &d.fonte[f];
      int ms = ptv_fonte_ms(s), dest = f == lenta && ms > 0;
      TxtLinha n, t;
      if (!s->ok && !s->falhas && linhas > 0) continue;
      n = dest ? txt_linha(TXT_ILHA_GENERO, ptv_fonte_rotulo(f), 232, 184, 74, 255) : dgTC(TXT_ILHA_GENERO, ptv_fonte_rotulo(f), 160);
      txt_desenhar_alpha(n, x, y + (passo - n.h) * 0.5f, dest ? 1.0f : 0.75f);
      { float bx = x + 176, bw = w - 176 - 316;
        gfx_cor((GfxRect){ bx, y + passo * 0.5f - 4, bw, 8 }, 0.5f, 1, 1, 1, 0.07f);
        if (ms > 0) {
          if (dest) gfx_cor((GfxRect){ bx, y + passo * 0.5f - 4, bw * ms / maior, 8 }, 0.5f, DG_AMBAR, 1);
          else gfx_cor((GfxRect){ bx, y + passo * 0.5f - 4, bw * ms / maior, 8 }, 0.5f, 0.953f, 0.949f, 0.937f, 0.5f);
        } }
      if (!s->ok && !s->falhas) snprintf(v, sizeof v, "%s", i18n("não medida"));
      else if (ms < 0) snprintf(v, sizeof v, "%s", i18n("falhou"));
      else if (s->iguais > 0 && f <= PTV_FONTE_FUNDO_MAX) snprintf(v, sizeof v, i18n("%d ms · igual ao card"), ms);
      else if (s->largura > 0) snprintf(v, sizeof v, i18n("%d ms · %d×%d · %d falhas"), ms, s->largura, s->altura, s->falhas);
      else snprintf(v, sizeof v, i18n("%d ms · %d falhas"), ms, s->falhas);
      t = txt_linha_corta(TXT_ILHA_APOIO, v, DG_TX, 300);
      txt_desenhar_alpha(t, x + w - t.w, y + (passo - t.h) * 0.5f, 0.6f);
      y += passo;
    } }
  y += 14;
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.07f);
  y += 14;
  { char a[220], b[220];
    a[0] = b[0] = 0;
    if (d.sugEstado == DS_PROPOSTA || d.sugEstado == DS_TESTANDO) {
      if (d.sug.motivo == PTV_MOTIVO_IGUAL && d.sug.alvo > 0)
        snprintf(a, sizeof a, i18n("Destaque em %s repete a imagem do card; %s é outra arte, %d ms."),
                 i18n(ptv_fonte_rotulo(d.sug.lenta)), i18n(ptv_fonte_rotulo(d.sug.alvo)), ptv_fonte_ms(&d.fonte[d.sug.alvo]));
      else snprintf(a, sizeof a, i18n("Destaque em %s: %d ms por arte; %s: %d ms."),
                    i18n(ptv_fonte_rotulo(d.sug.lenta)), d.sug.msLenta, i18n(ptv_fonte_rotulo(d.sug.base)), d.sug.msBase);
      if (!d.sug.diferente) snprintf(b, sizeof b, "%s", i18n("Vai mudar: Destaque com outra arte, de Ligado para Desligado."));
      else snprintf(b, sizeof b, i18n("Vai mudar: Background do hero, de %s para %s."),
                    i18n(nomeFonteAjuste(ajustes_hero_fonte())), i18n(nomeFonteAjuste(d.sug.fonte)));
      if (d.sugEstado == DS_TESTANDO) snprintf(b, sizeof b, "%s", i18n("Retestando a arte do destaque…"));
      { TxtLinha l = txt_linha_corta(TXT_ILHA_APOIO, a, 232, 184, 74, 255, w); txt_desenhar(l, x, y); }
      { TxtLinha l = dgTC(TXT_ILHA_APOIO, b, w); txt_desenhar_alpha(l, x, y + 24, 0.75f); }
    } else {
      const char *s = d.sugEstado == DS_MANTIDA ? "Sugestão aplicada: o destaque ficou mais rápido no reteste."
                    : d.sugEstado == DS_DESFEITA ? NULL : "A fonte do destaque não está mais lenta que a do card.";
      if (!s) { snprintf(a, sizeof a, i18n("Sugestão desfeita sozinha: %s"), i18n(d.sugMotivo ? d.sugMotivo : "")); s = a; }
      { TxtLinha l = dgTC(TXT_ILHA_APOIO, s, w); txt_desenhar_alpha(l, x, y, 0.55f); }
    } }
  // Tempo por etapa
  ajustes_ui_ilha(bl, 32, 0);
  x = bl.x + 34; w = bl.w - 68; y = bl.y + 30;
  ajustes_ui_kicker("Tempo por etapa", x, y, 1);
  { TxtLinha s = dgTC(TXT_ILHA_APOIO, i18n("Maior tempo agregado da amostra; falha isolada não derruba o addon."), w); txt_desenhar_alpha(s, x, y + 24, 0.45f); }
  y += 24 + 20 + 18;
  { static const char *const ROT[4] = { "Manifestos", "Catálogos", "Artes", "Fontes" };
    int t[4], maior = 1;
    float bw = (w - 3 * 22) / 4.0f, H = bl.h - 30 - 62 - 50 - 2 * 42.5f - 20;
    t[0] = d.manifestMs; t[1] = d.catalogMs; t[2] = d.assetsMs + d.fontesMs; t[3] = d.streamMs;
    for (i = 0; i < 4; i++) if (t[i] > maior) maior = t[i];
    for (i = 0; i < 4; i++) {
      float bx = x + i * (bw + 22), h = H * t[i] / maior;
      int m = t[i] == maior;
      char s[32];
      TxtLinha l, r;
      if (h < 6) h = 6;
      snprintf(s, sizeof s, "%.1f s", t[i] / 1000.0);
      if (!idioma_ponto_decimal(ajustes_idioma())) { char *p = strchr(s, '.'); if (p) *p = ','; }
      l = m ? txt_linha(TXT_AJ_CHIP, s, 232, 184, 74, 255) : dgT(TXT_AJ_CHIP, s);
      r = dgT(TXT_ILHA_APOIO, ROT[i]);
      if (m) gfx_cor((GfxRect){ bx, y + 26 + H - h, bw, h }, 12.0f / (h > 24 ? h : 24), DG_AMBAR, 0.85f);
      else gfx_cor((GfxRect){ bx, y + 26 + H - h, bw, h }, 12.0f / (h > 24 ? h : 24), 0.953f, 0.949f, 0.937f, 0.22f);
      txt_desenhar_alpha(l, bx + (bw - l.w) * 0.5f, y + 26 + H - h - 8 - l.h, m ? 1.0f : 0.6f);
      txt_desenhar_alpha(r, bx + (bw - r.w) * 0.5f, y + 26 + H + 8, 0.55f);
    }
    y += 26 + H + 8 + 20 + 12; }
  { long bytes = 0, lim = tex_orcamento_bytes(); int pend = 0, quentes = 0;
    tex_estatisticas(NULL, &pend, &bytes, &quentes, NULL);
    snprintf(v, sizeof v, "%ld / %ld MB", bytes >> 20, lim > 0 ? lim >> 20 : 0L);
    y += dgStat("Memória usada por imagens", v, x, y, w);
    snprintf(v, sizeof v, i18n("%d pendentes · %d quentes"), pend, quentes);
    dgStat("Fila de imagens", v, x, y, w); }
  // O que foi aplicado
  ajustes_ui_ilha(br, 32, 0);
  x = br.x + 34; w = br.w - 68; y = br.y + 30;
  ajustes_ui_kicker("Agora nesta TV", x, y, 1);
  { TxtLinha m = dgT(TXT_ILHA_APOIO, d.modo == DIAG_DESEMPENHO ? "Desempenho" : "Qualidade");
    txt_desenhar_alpha(m, x + w - m.w, y, 0.5f); }
  y += 18 + 12;
  { const char *txtAp = textoAplicacao(&cor);
    TxtLinha t = cor == 1 ? txt_linha_corta(TXT_ILHA_NOME, txtAp, 76, 195, 138, 255, w - 20)
               : cor == 2 ? txt_linha_corta(TXT_ILHA_NOME, txtAp, 232, 184, 74, 255, w - 20) : dgTC(TXT_ILHA_NOME, txtAp, w - 20);
    if (cor == 1) dgPonto(x, y + t.h * 0.5f, DG_VERDE);
    else if (cor == 2) dgPonto(x, y + t.h * 0.5f, DG_AMBAR);
    txt_desenhar(t, x + (cor ? 17 : 0), y);
    y += t.h + 6;
    if (d.motivo) { TxtLinha m = dgTC(TXT_ILHA_APOIO, d.motivo, w - 17); txt_desenhar_alpha(m, x + 17, y, 0.55f); y += 22; } }
  y += 8;
  { int pior = d.aplicacao == DA_RESTAURADO_AUTO;
    PtvPerfil a, b;
    float hw = (w - 32) * 0.5f, yy = y;
    perfilDoResultado(&a, &b);
    yy += ajustes_ui_antes_depois("Memória para imagens agora", a.texMb, b.texMb, "MB", 512, x, yy, hw, 1, pior ? 0.910f : -1, 0.722f, 0.290f);
    yy += ajustes_ui_antes_depois("Fios de rede das artes", a.fiosRede, b.fiosRede, "fios", 6, x, yy, hw, 1, pior ? 0.910f : -1, 0.722f, 0.290f);
    ajustes_ui_antes_depois("Largura do fundo em tela cheia", a.heroiLarg, b.heroiLarg, "px", 2560, x, yy, hw, 1, pior ? 0.910f : -1, 0.722f, 0.290f);
    if (d.medAntes.artesMs || d.medDepois.artesMs) {
      int V[3] = { d.medFrio.artesMs, d.medAntes.artesMs, d.medDepois.artesMs }, mx = 1, k;
      static const char *const N[3] = { "Cache frio", "Antes", "Depois" };
      float cx = x + hw + 32, cy = y + 10;
      ajCapsDg("Artes, mesma amostra", cx, cy);
      cy += 22;
      for (k = 0; k < 3; k++) if (V[k] > mx) mx = V[k];
      for (k = 0; k < 3; k++) {
        TxtLinha n = dgT(TXT_ILHA_APOIO, N[k]), t;
        char ms[32];
        float bx = cx + 92, bw = hw - 92 - 96;
        snprintf(ms, sizeof ms, "%d ms", V[k]);
        t = dgT(TXT_ILHA_APOIO, ms);
        txt_desenhar_alpha(n, cx, cy + (32 - n.h) * 0.5f, 0.55f);
        gfx_cor((GfxRect){ bx, cy + 11, bw, 10 }, 0.5f, 1, 1, 1, 0.06f);
        if (k == 2) { if (pior) gfx_cor((GfxRect){ bx, cy + 11, bw * V[k] / mx, 10 }, 0.5f, DG_AMBAR, 1);
                      else gfx_cor((GfxRect){ bx, cy + 11, bw * V[k] / mx, 10 }, 0.5f, DG_VERDE, 1); }
        else gfx_cor((GfxRect){ bx, cy + 11, bw * V[k] / mx, 10 }, 0.5f, 0.953f, 0.949f, 0.937f, k ? 0.5f : 0.25f);
        txt_desenhar_alpha(t, cx + hw - t.w, cy + (32 - t.h) * 0.5f, 0.75f);
        cy += 32;
      }
    } }
  // Botoes
  { int lista[B_N], n = botoesVisiveis(lista);
    float bx = X, by = NV_TELA_H - 40 - 60;
    if (d.botao >= n) d.botao = n > 0 ? n - 1 : 0;
    for (i = 0; i < n; i++) bx += ajustes_ui_botao(i18n(BOTAO_ROTULO[lista[i]]), NULL, bx, by, i == d.botao) + 12;
    { const char *k[3] = { "← →", "OK", "Voltar" }, *l[3] = { "Escolher", "Confirmar", "Sair" };
      float dw = ajustes_ui_dicas(k, l, 3, 0, 0, 0);
      ajustes_ui_dicas(k, l, 3, NV_TELA_W - 40 - dw, by + 15, 1); } }
}

// --- TESTE DE VELOCIDADE (mockup stats-velocidade, telas 4 e 5, 07/10) ----------
// Uma ilha so, larga: a esquerda o numero grande e o grafico (um ponto por
// segundo ao vivo; no resultado, todas as amostras), a direita os passos,
// latencia e jitter e as fontes medidas (rodando) ou a tabela do que a
// velocidade toca, o veredito e os botoes (resultado). Chips neutros, foco
// pela peca de Ajustes (ajustes_ui_botao), icones Lucide.
#define VZ_TOPO 112.0f
static float vzX0(void) { return dgX0(); }
static float vzW(void) { return NV_TELA_W - 48.0f - vzX0(); }
static void vzCartao(GfxRect r) { gfx_cor(r, 22.0f / r.h, 1, 1, 1, 0.045f); }

// Chip neutro (o .chip do mockup): fundo branco a 8%, fio de 8%, 20/600.
static float vzChip(const char *icone, const char *s, float x, float y, int desenha) {
  TxtLinha t = txt_linha(TXT_AJ_SEG, s, 216, 216, 216, 255);
  float w = 18 + (icone ? 30 : 0) + t.w + 18;
  if (!desenha) return w;
  gfx_cor((GfxRect){ x, y, w, 40 }, 0.5f, 1, 1, 1, 0.08f);
  gfx_anel((GfxRect){ x, y, w, 40 }, 0.5f, 1.0f, 1, 1, 1, 0.08f);
  if (icone) gfx_icone((GfxRect){ x + 18, y + 10, 20, 20 }, icone, 0.85f, 0.85f, 0.85f, 1);
  txt_desenhar(t, x + 18 + (icone ? 30 : 0), y + (40 - t.h) * 0.5f);
  return w;
}

// Cabecalho comum: kicker, titulo e o chip da direita.
static void vzCabecalho(GfxRect p, const char *icone, const char *chip) {
  float x = p.x + 48;
  ajustes_ui_kicker("Ajustes · Diagnóstico", x, p.y + 40, 1);
  { TxtLinha t = dgT(TXT_V2_TIT, "Teste de velocidade"); txt_desenhar(t, x, p.y + 62); }
  if (chip && chip[0]) {
    float w = vzChip(icone, chip, 0, 0, 0);
    vzChip(icone, chip, p.x + p.w - 48 - w, p.y + 54, 1);
  }
}

// Linha com area: `v` em kbps, `total` pontos no eixo (o grafico enche da
// esquerda para a direita), escala `mx` kbps. `doZero`: o 1o segmento sobe do
// chao (ao vivo, o teste comeca do nada). A area vai em fatias de ~6 px
// interpoladas, para a borda de cima seguir a linha sem degrau.
static void vzLinha(const int *v, int n, int total, float mx, GfxRect r, float esp, int doZero) {
  float px, py;
  int i, den;
  if (n < 1 || mx <= 0) return;
  den = doZero ? (total - 1 > n ? total - 1 : n) : (total - 1 > n - 1 ? total - 1 : n - 1);
  if (den < 1) den = 1;
  px = r.x;
  py = doZero ? r.y + r.h : r.y + r.h - r.h * fminf((float)v[0] / mx, 1.0f);
  for (i = doZero ? 0 : 1; i < n; i++) {
    float x = r.x + r.w * (float)(doZero ? i + 1 : i) / (float)den;
    float y = r.y + r.h - r.h * fminf((float)v[i] / mx, 1.0f);
    float sx;
    for (sx = px; sx < x - 0.5f; sx += 6.0f) {
      float sw = fminf(6.0f, x - sx), t = (sx + sw * 0.5f - px) / (x - px);
      float sy = py + (y - py) * t;
      gfx_cor((GfxRect){ sx, sy, sw, r.y + r.h - sy }, 0, 1, 1, 1, 0.10f);
    }
    dgLinha(px, py, x, y, esp, 0.953f, 0.949f, 0.937f, 1);
    px = x; py = y;
  }
}

// "Rotulo ...... valor" de 40 px com fio, no corpo do mockup (22).
static float vzKv(const char *k, const char *v, float x, float y, float w) {
  TxtLinha tv = dgTC(TXT_G22M, v, w * 0.7f), tk = txt_linha_corta(TXT_AJ_TEXTO, k, 154, 154, 156, 255, w - tv.w - 16);
  gfx_cor((GfxRect){ x, y, w, 1 }, 0, 1, 1, 1, 0.08f);
  txt_desenhar(tk, x, y + (40 - tk.h) * 0.5f);
  txt_desenhar(tv, x + w - tv.w, y + (40 - tv.h) * 0.5f);
  return 40;
}

// Numero grande com a unidade na linha de base. Devolve a altura usada.
static float vzNumero(const char *n, const char *u, float x, float y) {
  TxtLinha t = dgT(TXT_AJ_NUM110, n), un = txt_linha(TXT_V2_ROT, u, 180, 180, 182, 255);
  txt_desenhar(t, x, y);
  txt_desenhar(un, x + t.w + 14, y + t.h * 0.78f - un.h);
  return t.h;
}

// --- RODANDO (tela 4) ------------------------------------------------------------
static void dgVazaoAoVivo(Uint32 agora) {
  GfxRect p = { vzX0(), VZ_TOPO, vzW(), NV_TELA_H - VZ_TOPO - 50 };
  float x = p.x + 48, lw = (p.w - 96 - 40) * 0.6f, rx = x + lw + 40, rw = p.x + p.w - 48 - rx;
  int fase = atomic_load(&vz.fase), nf = atomic_load(&vz.nFonte), i;
  int baixando = fase == 2 && vz.fonteIniMs;
  char a[200], b[32];
  (void)agora;
  ajustes_ui_ilha(p, 36, 0);
  { const char *h = vz.hostAgora[0] ? vz.hostAgora : vz.latHost;
    if (h[0]) snprintf(a, sizeof a, i18n("Testando com o seu add-on · %s"), h);
    else snprintf(a, sizeof a, "%s", i18n("Testando com o seu add-on"));
    vzCabecalho(p, "aj_server", a); }

  // ESQUERDA: download agora, pico e mediana, o grafico e a barra da fonte.
  { GfxRect c = { x, p.y + 150, lw, 600 };
    float cx = c.x + 34, cw = c.w - 68, y;
    int mx = 0, ult = vz.nVivo ? vz.vivo[vz.nVivo - 1] : -1;
    vzCartao(c);
    ajCapsDg("Download agora", cx, c.y + 30);
    if (vz.nVivo) {
      int ord[VAZ_VIVO_MAX], k, med;
      float xr = cx + cw;
      for (k = 0; k < vz.nVivo; k++) { ord[k] = vz.vivo[k]; if (vz.vivo[k] > mx) mx = vz.vivo[k]; }
      for (k = 1; k < vz.nVivo; k++) { int t = ord[k], j = k; while (j > 0 && ord[j - 1] > t) { ord[j] = ord[j - 1]; j--; } ord[j] = t; }
      med = (vz.nVivo & 1) ? ord[vz.nVivo / 2] : (ord[vz.nVivo / 2 - 1] + ord[vz.nVivo / 2]) / 2;
      ev_fmt_mbps(b, sizeof b, med, sepDecimal());
      snprintf(a, sizeof a, i18n("Mediana %s"), b);
      xr -= vzChip(NULL, a, 0, 0, 0); vzChip(NULL, a, xr, c.y + 34, 1);
      ev_fmt_mbps(b, sizeof b, mx, sepDecimal());
      snprintf(a, sizeof a, i18n("Pico %s"), b);
      xr -= 12 + vzChip(NULL, a, 0, 0, 0); vzChip(NULL, a, xr, c.y + 34, 1);
    }
    ev_fmt_mbps(b, sizeof b, ult, sepDecimal());
    vzNumero(b, "Mbps", cx - 4, c.y + 56);
    y = c.y + 218;
    { float H = 298, esc = mx > 0 ? (float)mx * 1.25f : 10000.0f;
      int k;
      for (k = 0; k < 4; k++) gfx_cor((GfxRect){ cx, y + H * k / 4.0f, cw, 1 }, 0, 1, 1, 1, 0.07f);
      vzLinha(vz.vivo, vz.nVivo, VAZAO_FONTES_MAX * VAZ_JANELA_S, esc, (GfxRect){ cx, y, cw, H }, 2.5f, 1);
      y += H + 12; }
    { float dentro = baixando ? (float)(SDL_GetTicks() - vz.fonteIniMs) / 1000.0f : 0.0f;
      int s = (int)dentro;
      if (s > VAZ_JANELA_S) s = VAZ_JANELA_S;
      gfx_cor((GfxRect){ cx, y, cw, 10 }, 0.5f, 1, 1, 1, 0.08f);
      if (dentro > 0) gfx_cor((GfxRect){ cx, y, cw * fminf(dentro / VAZ_JANELA_S, 1.0f), 10 }, 0.5f, 0.902f, 0.902f, 0.894f, 1);
      snprintf(a, sizeof a, i18n("%d de %d s · cada segundo é um ponto do gráfico; os dados baixados são descartados"),
               s, VAZ_JANELA_S);
      { TxtLinha l = dgTC(TXT_AJ_TEXTO, a, cw); txt_desenhar_alpha(l, cx, y + 24, 0.62f); } } }

  // DIREITA: os tres passos.
  { float y = p.y + 150;
    for (i = 0; i < 3; i++) {
      int feito = i == 0 ? fase == 2 : 0, atual = i == 0 ? fase != 2 : i == 1 ? fase == 2 : 0;
      float cy = y + 17 + i * 48;
      GfxRect o = { rx, cy - 17, 34, 34 };
      TxtLinha t;
      if (i == 1) {
        int k = nf + 1 > VAZAO_FONTES_MAX ? VAZAO_FONTES_MAX : nf + 1;
        snprintf(a, sizeof a, i18n("Download · fonte %d de %d"), k, VAZAO_FONTES_MAX);
      } else snprintf(a, sizeof a, "%s", i18n(i == 0 ? "Latência e jitter" : "Estabilidade e veredito"));
      if (feito) {
        gfx_cor(o, 0.5f, 0.902f, 0.902f, 0.894f, 1);
        gfx_icone((GfxRect){ o.x + 8, o.y + 8, 18, 18 }, "aj_check", 0.08f, 0.08f, 0.08f, 1);
      } else if (atual) {
        gfx_anel(o, 0.5f, 3.0f, 0.953f, 0.949f, 0.937f, 1);
        gfx_icone((GfxRect){ o.x + 8, o.y + 8, 18, 18 }, i == 1 ? "aj_arrow-down" : "aj_activity", 0.953f, 0.949f, 0.937f, 1);
      } else {
        char n[4];
        TxtLinha l;
        snprintf(n, sizeof n, "%d", i + 1);
        l = dgT(TXT_AJ_KBD, n);
        gfx_anel(o, 0.5f, 2.0f, 0.29f, 0.29f, 0.306f, 1);
        txt_desenhar_alpha(l, o.x + (34 - l.w) * 0.5f, o.y + (34 - l.h) * 0.5f, 0.55f);
      }
      t = atual ? dgT(TXT_AJ_SECAO, a) : dgT(TXT_AJ_TEXTO, a);
      txt_desenhar_alpha(t, rx + 46, cy - t.h * 0.5f, atual ? 1.0f : feito ? 0.82f : 0.55f);
    } }

  // Latencia e jitter.
  { float tw = (rw - 24) * 0.5f, y = p.y + 318;
    for (i = 0; i < 2; i++) {
      GfxRect c = { rx + i * (tw + 24), y, tw, 214 };
      int v = i ? vz.jitMs : vz.latMs, tem = atomic_load(&vz.latPronta) && vz.latN > 0;
      vzCartao(c);
      gfx_cor((GfxRect){ c.x + 24, c.y + 24, 44, 44 }, 12.0f / 44.0f, 1, 1, 1, 0.07f);
      dgIcone(i ? "aj_activity" : "aj_zap", c.x + 24 + 10, c.y + 24 + 10, 24, 1);
      { TxtLinha l = txt_linha(TXT_G22M, i ? "Jitter" : "Latência", 180, 180, 182, 255);
        txt_desenhar(l, c.x + 24 + 58, c.y + 24 + (44 - l.h) * 0.5f); }
      if (tem) snprintf(b, sizeof b, "%d", v); else snprintf(b, sizeof b, "—");
      { TxtLinha n = dgT(TXT_V2_NUM, b), u = txt_linha(TXT_V2_LN, "ms", 180, 180, 182, 255);
        txt_desenhar(n, c.x + 24, c.y + 84);
        if (tem) txt_desenhar(u, c.x + 24 + n.w + 8, c.y + 84 + n.h * 0.8f - u.h); }
      dgBloco(TXT_AJ_TEXTO, !tem ? (atomic_load(&vz.latPronta) ? "sem resposta do servidor" : "medindo…")
                        : i ? "variação entre as respostas" : "até o servidor, mediana de 10",
              c.x + 24, c.y + 150, tw - 48, 28, 0.62f, 2);
    } }

  // Fontes medidas.
  { GfxRect c = { rx, p.y + 556, rw, 226 };
    float y = c.y + 92;
    vzCartao(c);
    gfx_cor((GfxRect){ c.x + 24, c.y + 24, 44, 44 }, 12.0f / 44.0f, 1, 1, 1, 0.07f);
    dgIcone("aj_server", c.x + 24 + 10, c.y + 24 + 10, 24, 1);
    { TxtLinha l = txt_linha(TXT_G22M, "Fontes medidas", 180, 180, 182, 255);
      txt_desenhar(l, c.x + 24 + 58, c.y + 24 + (44 - l.h) * 0.5f); }
    for (i = 0; i < VAZAO_FONTES_MAX; i++) {
      const char *host = i < nf ? vz.hostFonte[i] : i == nf && baixando ? vz.hostAgora : "";
      int ad = i < nf ? vz.addonFonte[i] : vz.addonAgora;
      TxtLinha v, h, n;
      if (i < nf) {
        ev_fmt_mbps(b, sizeof b, vz.fonte[i].r.medianaKbps, sepDecimal());
        snprintf(a, sizeof a, "%s Mbps", b);
        v = dgT(TXT_G23B, a);
      } else v = txt_linha(TXT_G23B, i == nf && baixando ? "medindo…" : "na fila", 180, 180, 182, 255);
      if (i > 0) gfx_cor((GfxRect){ c.x + 24, y - 1, c.w - 48, 1 }, 0, 1, 1, 1, 0.07f);
      if (host[0] || i <= nf) {
        snprintf(a, sizeof a, "%s", host[0] ? host : "…");
        h = dgTC(TXT_AJ_TEXTO, a, (c.w - 48 - v.w - 24) * 0.62f);
        txt_desenhar_alpha(h, c.x + 24, y + (40 - h.h) * 0.5f, 0.75f);
        if (ad >= 0 && ad < addons_n() && (i < nf || (i == nf && baixando))) {
          snprintf(a, sizeof a, "· %s", addons_nome(ad));
          n = txt_linha_corta(TXT_AJ_TEXTO, a, 154, 154, 156, 255, c.w - 48 - v.w - 24 - h.w - 8);
          txt_desenhar(n, c.x + 24 + h.w + 8, y + (40 - n.h) * 0.5f);
        }
      } else {
        snprintf(a, sizeof a, i18n("Fonte %d"), i + 1);
        h = dgT(TXT_AJ_TEXTO, a);
        txt_desenhar_alpha(h, c.x + 24, y + (40 - h.h) * 0.5f, 0.55f);
      }
      txt_desenhar(v, c.x + c.w - 24 - v.w, y + (40 - v.h) * 0.5f);
      y += 40;
    } }

  ajustes_ui_botao(i18n("Cancelar"), "aj_x", x, p.y + p.h - 48 - 60, 0);
}

// Frase com o comeco em negrito ("4K HDR: ok.") e o resto na mesma linha,
// quebrando embaixo na largura do cartao.
static void vzFrase(const char *forte, const char *resto, float x, float y, float w) {
  TxtLinha f = dgT(TXT_G28B, forte), l;
  char l1[240] = "", t[240];
  const char *p = resto, *corte = resto;
  float w1 = w - f.w - 10;
  while (*p) {
    const char *e = strchr(p, ' ');
    int len = e ? (int)(e - p) : (int)strlen(p);
    snprintf(t, sizeof t, "%s%s%.*s", l1, l1[0] ? " " : "", len, p);
    if (txt_largura(TXT_V2_28, t) > w1) break;
    snprintf(l1, sizeof l1, "%s", t);
    p += len;
    while (*p == ' ') p++;
    corte = p;
  }
  txt_desenhar(f, x, y);
  l = dgT(TXT_V2_28, l1);
  if (l1[0]) txt_desenhar(l, x + f.w + 10, y + (f.h - l.h) * 0.5f);
  if (*corte) txt_bloco(TXT_V2_28, corte, 243, 242, 239, x, y + 36, w, 34, 1, 2);
}

// --- RESULTADO (tela 5) ------------------------------------------------------------
static void dgVazaoResultado(Uint32 agora) {
  GfxRect p = { vzX0(), VZ_TOPO, vzW(), NV_TELA_H - VZ_TOPO - 50 };
  float x = p.x + 48, lw = (p.w - 96 - 40) * 0.42f, rx = x + lw + 40, rw = p.x + p.w - 48 - rx;
  char a[240], b[32], c2[32];
  int i, bom, prox, nf = atomic_load(&vz.nFonte);
  (void)agora;
  ajustes_ui_ilha(p, 36, 0);
  if (vz.envioPedido && atomic_load(&d.enviando)) snprintf(a, sizeof a, "%s", i18n("Enviando relatório…"));
  else if (vz.envioPedido && d.enviado) snprintf(a, sizeof a, "%s", i18n("Relatório enviado"));
  else if (vz.envioPedido && d.envioFalhou) snprintf(a, sizeof a, "%s", i18n("O envio falhou"));
  else snprintf(a, sizeof a, i18n("Concluído · %02d:%02d"), vz.fimHora, vz.fimMin);
  vzCabecalho(p, vz.envioPedido ? "aj_send" : "aj_clock", a);

  // ESQUERDA: sua conexao.
  { GfxRect c = { x, p.y + 150, lw, 620 };
    float cx = c.x + 36, cw = c.w - 72, y;
    vzCartao(c);
    ajCapsDg("Sua conexão", cx, c.y + 38);
    ev_fmt_mbps(b, sizeof b, vz.resumo.medianaKbps, sepDecimal());
    y = c.y + 64 + vzNumero(b, "Mbps", cx - 4, c.y + 64) - 4;
    ev_fmt_mbps(c2, sizeof c2, vz.resumo.p20Kbps, sepDecimal());
    if (nf == 1) snprintf(a, sizeof a, i18n("1 fonte · pior trecho %s Mbps"), c2);
    else snprintf(a, sizeof a, i18n("mediana de %d fontes · pior trecho %s Mbps"), nf, c2);
    { TxtLinha l = dgTC(TXT_V2_24, a, cw); txt_desenhar_alpha(l, cx, y, 0.72f); y += l.h + 30; }
    { int mx = 1, k;
      for (k = 0; k < vz.nAmostra; k++) if (vz.amostra[k] > mx) mx = vz.amostra[k];
      vzLinha(vz.amostra, vz.nAmostra, vz.nAmostra, (float)mx * 1.4f, (GfxRect){ cx, y, cw, 102 }, 2.0f, 0);
      y += 102 + 26; }
    snprintf(b, sizeof b, "%d ms", vz.latMs);
    snprintf(c2, sizeof c2, "%d ms", vz.jitMs);
    y += vzKv(i18n("Latência"), vz.latN ? b : "—", cx, y, cw);
    y += vzKv(i18n("Jitter"), vz.latN ? c2 : "—", cx, y, cw);
    { char go[16], gm[16];
      vazao_fmt_gb(go, sizeof go, vazao_gb(vz.resumo.otimoKbps, VAZAO_FILME_S), sepDecimal());
      vazao_fmt_gb(gm, sizeof gm, vazao_gb(vz.resumo.maximoKbps, VAZAO_FILME_S), sepDecimal());
      snprintf(a, sizeof a, i18n("até %s GB / %s GB por filme de 2 h"), go, gm);
      vzKv(i18n("Arquivo ideal / máximo"), a, cx, y, cw); } }

  // DIREITA: o que essa velocidade toca.
  { float y = p.y + 150;
    ajCapsDg("O que essa velocidade toca", rx, y);
    y += 34;
    for (i = 0; i < VAZ_QUALIDADES; i++) {
      const VazQualidade *q = vazao_qualidade(i);
      VazBanda bd = vazao_banda(&vz.resumo, q->tipicoKbps);
      GfxRect r = { rx, y, rw, 78 };
      static const char *const ROT[3] = { "Toca bem", "Pode travar", "Não toca" };
      static const char *const IC[3] = { "aj_check", "aj_triangle-alert", "aj_x" };
      float al = bd == VBANDA_NAO_TOCA ? 0.55f : 1.0f;
      vzCartao(r);
      gfx_cor((GfxRect){ r.x + 20, r.y + 17, 44, 44 }, 12.0f / 44.0f, 1, 1, 1, 0.07f);
      dgIcone(q->icone, r.x + 20 + 11, r.y + 17 + 11, 22, al);
      { TxtLinha n = dgT(TXT_G28B, q->nome);
        char tp[32];
        TxtLinha s;
        snprintf(tp, sizeof tp, i18n("%d Mbps típico"), q->tipicoKbps / 1000);
        s = txt_linha(TXT_AJ_18, tp, 154, 154, 156, 255);
        txt_desenhar_alpha(n, r.x + 84, r.y + 10, al);
        txt_desenhar_alpha(s, r.x + 84, r.y + 12 + n.h, al); }
      { TxtLinha t = bd == VBANDA_TOCA ? txt_linha(TXT_G21B, ROT[bd], 20, 20, 20, 255)
                   : bd == VBANDA_PODE_TRAVAR ? dgT(TXT_G21B, ROT[bd])
                   : txt_linha(TXT_G21B, ROT[bd], 154, 154, 156, 255);
        float pw = 18 + 20 + 10 + t.w + 18, px = r.x + r.w - 20 - pw, py = r.y + 19;
        GfxRect pr = { px, py, pw, 40 };
        if (bd == VBANDA_TOCA) gfx_cor(pr, 0.5f, 0.949f, 0.949f, 0.933f, 1);
        else if (bd == VBANDA_PODE_TRAVAR) { gfx_cor(pr, 0.5f, 1, 1, 1, 0.10f); gfx_anel(pr, 0.5f, 2.0f, 0.435f, 0.435f, 0.447f, 1); }
        else gfx_anel(pr, 0.5f, 2.0f, 0.227f, 0.227f, 0.235f, 1);
        if (bd == VBANDA_TOCA) gfx_icone((GfxRect){ px + 18, py + 10, 20, 20 }, IC[bd], 0.08f, 0.08f, 0.08f, 1);
        else gfx_icone((GfxRect){ px + 18, py + 10, 20, 20 }, IC[bd], 0.85f, 0.85f, 0.85f, bd == VBANDA_NAO_TOCA ? 0.7f : 1);
        txt_desenhar(t, px + 18 + 30, py + (40 - t.h) * 0.5f); }
      y += 78 + 10;
    }
    // O VEREDITO, em palavras: o melhor que toca bem e o que vem logo acima.
    { GfxRect v = { rx, y + 10, rw, 104 };
      char go[16], forte[96], resto[240];
      TxtLinha f = { 0 };
      vzCartao(v);
      vazao_veredito(&vz.resumo, &bom, &prox);
      vazao_fmt_gb(go, sizeof go, vazao_gb(vz.resumo.otimoKbps, VAZAO_FILME_S), sepDecimal());
      if (bom >= 0) snprintf(forte, sizeof forte, i18n("%s: ok."), vazao_qualidade(bom)->nome);
      else snprintf(forte, sizeof forte, "%s", i18n("Abaixo de 1080p."));
      if (prox < 0) snprintf(resto, sizeof resto, "%s", i18n("Até o remux mais pesado toca sem parar."));
      else if (vazao_banda(&vz.resumo, vazao_qualidade(prox)->tipicoKbps) == VBANDA_PODE_TRAVAR)
        snprintf(resto, sizeof resto, i18n("%s pode travar nas cenas mais pesadas; prefira arquivos de até %s GB."),
                 vazao_qualidade(prox)->nome, go);
      else snprintf(resto, sizeof resto, i18n("%s não toca sem parar; prefira arquivos de até %s GB."),
                    vazao_qualidade(prox)->nome, go);
      (void)f;
      vzFrase(forte, resto, v.x + 22, v.y + 18, v.w - 44); }
  }
  // Botoes.
  { int lista[VB_N], n = vazBotoes(lista);
    float bx = rx, by = p.y + p.h - 48 - 60 - 40;
    if (vz.botao >= n) vz.botao = n - 1;
    for (i = 0; i < n; i++)
      bx += ajustes_ui_botao(i18n(VB_ROTULO[lista[i]]), VB_ICONE[lista[i]], bx, by, i == vz.botao) + 14;
    { const char *k[1] = { "Voltar" }, *l[1] = { "Sair" };
      float dw = ajustes_ui_dicas(k, l, 1, 0, 0, 0);
      ajustes_ui_dicas(k, l, 1, p.x + p.w - 48 - dw, p.y + p.h - 44, 1); } }
}

// O resto do teste de velocidade (rodando, sem medida, ciclo completo): os
// paineis de sempre, dentro das ilhas.
static void dgVazaoGenerico(Uint32 agora) {
  GfxRect e = { dgX0(), 112, 900, NV_TELA_H - 152 }, dr = { 0, 112, 0, NV_TELA_H - 152 };
  float ar, ag, ab;
  int ve = atomic_load(&vz.estado);
  ajustes_acento(&ar, &ag, &ab);
  dr.x = e.x + e.w + 24; dr.w = NV_TELA_W - 40 - dr.x;
  e.h -= 84;
  dgCabecalho(i18n("Teste de velocidade"), i18n("Desempenho desta TV"));
  ajustes_ui_ilha(e, 36, 0);
  ajustes_ui_ilha(dr, 36, 0);
  desenharVazaoResumo(e, ar, ag, ab, agora);
  if (vz.ciclo != VCM_RAPIDO) desenharCicloRanking(dr, ar, ag, ab);
  else desenharVazaoOrigem(dr, ar, ag, ab);
  if (ve != 1) {
    int lista[VB_N], n = vazBotoes(lista), i;
    float bx = e.x, by = NV_TELA_H - 40 - 60;
    if (vz.botao >= n) vz.botao = n - 1;
    for (i = 0; i < n; i++) bx += ajustes_ui_botao(i18n(VB_ROTULO[lista[i]]), NULL, bx, by, i == vz.botao) + 12;
  } else {
    const char *k[1] = { "Voltar" }, *l[1] = { "Cancelar" };
    ajustes_ui_dicas(k, l, 1, e.x, NV_TELA_H - 40 - 45, 1);
  }
}
