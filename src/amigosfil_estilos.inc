// Alternative friends row layouts. Classic rendering stays in amigosfil.c.
#define AF_FEED_MAX (SV_AMIGOS_MAX * 2)
static struct { int amigo, titulo; } afItens[AF_FEED_MAX];
static int afN, afEstilo = -1;
static unsigned afRev = ~0u;
static GfxRect afFoco;
static int afExemplos, afConviteEntrou;
static char afId[96], afImdb[24];

static const SvAmigo *afAmigo(int i) {
  return i >= 0 && i < afN ? socialvis_amigo(afItens[i].amigo) : NULL;
}
static const SvEvento *afEvento(int i) {
  const SvAmigo *a = afAmigo(i);
  return a && afItens[i].titulo >= 0 ? &a->tit[afItens[i].titulo] : NULL;
}
static void afMontar(void) {
  int estilo = ajustes_amigos_estilo();
  unsigned rev = socialvis_revisao();
  if (afRev == rev && afEstilo == estilo) return;
  int mudouEstilo = afEstilo != estilo;
  afRev = rev; afEstilo = estilo; afN = 0;
  if (mudouEstilo) { scroll = vScroll = 0.0f; dentro = -1; colAnt = -1; afId[0] = afImdb[0] = 0; }
  int usados[SV_AMIGOS_MAX] = {0};
  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < socialvis_n_amigos(); i++) {
      const SvAmigo *a = socialvis_amigo(i);
      int max = estilo == 2 || estilo == 3 ? 1 : 2;
      if (!a->nTit && pass == 1 && afN < AF_FEED_MAX) {
        afItens[afN].amigo = i; afItens[afN++].titulo = -1;
      }
      for (int j = 0; j < a->nTit && usados[i] < max && afN < AF_FEED_MAX; j++) {
        const SvEvento *e = &a->tit[j];
        if ((e->acao == SV_AGORA) != (pass == 0)) continue;
        if (estilo == 4) {
          int repetido = 0;
          for (int k = 0; k < afN; k++) {
            const SvEvento *other = afEvento(k);
            if (other && e->imdb[0] && !strcmp(e->imdb, other->imdb)) repetido = 1;
          }
          if (repetido) continue;
        }
        afItens[afN].amigo = i; afItens[afN++].titulo = j; usados[i]++;
      }
    }
  }
  // Live first, then newest known events, preserving stable ties.
  if (estilo != 2 && estilo != 3) for (int i = 1; i < afN; i++) {
    for (int j = i; j > 0; j--) {
      const SvEvento *a = afEvento(j), *b = afEvento(j - 1);
      int liveA = a && a->acao == SV_AGORA, liveB = b && b->acao == SV_AGORA;
      if (liveA < liveB || (liveA == liveB && (!a || (b && a->quando <= b->quando)))) break;
      int am = afItens[j].amigo, ti = afItens[j].titulo;
      afItens[j] = afItens[j - 1];
      afItens[j - 1].amigo = am; afItens[j - 1].titulo = ti;
    }
  }
}
static int afNum(void) { afMontar(); return afN; }
static int afVivos(void) {
  int n = 0;
  while (n < afN && afEvento(n) && afEvento(n)->acao == SV_AGORA) n++;
  return n;
}
static GfxRect afRect(int i, float x0, float y) {
  int s = ajustes_amigos_estilo();
  GfxRect r = { x0, y + 8.0f, NV_DESTAQUE_W, NV_DESTAQUE_H };
  if (s == 1) {
    int vivos = afVivos();
    if (i < vivos) { r.x += i * 584.0f; r.w = 560.0f; r.h = 236.0f; }
    else {
      int k = i - vivos;
      r.x += vivos * 584.0f + (k / 3) * 504.0f;
      r.y += 38.0f + (k % 3) * 86.0f;
      r.w = 480.0f; r.h = 76.0f;
    }
  } else if (s == 2 || s == 4) {
    r.x += i * 228.0f; r.w = 204.0f; r.h = 306.0f;
  } else if (s == 3) {
    r.x += i * 408.0f; r.w = 384.0f; r.h = 96.0f;
  } else r.x += i * (NV_DESTAQUE_W + 24.0f);
  return r;
}
static GfxRect afAcaoRect(int i, float y) {
  float w = 224.0f, dir = amigosfilDireita();
  return (GfxRect){dir - (i == afN ? 2.0f * w + 16.0f : w), y - 55.0f, w, 42.0f};
}
static void afTexto(const char *s, float x, float y, float w, int fonte, float alpha) {
  TxtLinha t = txt_linha_corta(fonte, s, 238, 238, 244, 255, w);
  txt_desenhar_alpha(t, x, y, alpha);
}
static void afAnel(GfxRect r, int foco) {
  if (!foco) return;
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  GfxRect b = {r.x - NV_ANEL_FOCO, r.y - NV_ANEL_FOCO,
               r.w + 2 * NV_ANEL_FOCO, r.h + 2 * NV_ANEL_FOCO};
  gfx_cor(b, 14.0f / b.h, ar, ag, ab, 1.0f);
  afFoco = b;
}
static void afLegenda(int i, GfxRect r, float alpha) {
  const SvAmigo *a = afAmigo(i);
  const SvEvento *e = afEvento(i);
  char linha[256], status[160];
  snprintf(linha, sizeof linha, "%s%s%s", a->nome, e ? " " : "", e ? socialvis_verbo(e) : "");
  if (ajustes_amigos_estilo() == 4 && e) {
    linha[0] = 0;
    for (int k = 0; k < socialvis_n_amigos(); k++) {
      const SvAmigo *other = socialvis_amigo(k);
      for (int j = 0; j < other->nTit; j++) if (e->imdb[0] && !strcmp(other->tit[j].imdb, e->imdb)) {
        size_t n = strlen(linha);
        snprintf(linha + n, sizeof linha - n, "%s%s", n ? ", " : "", other->nome);
        break;
      }
    }
  }
  afTexto(linha, r.x, r.y + r.h + 12.0f, r.w, TXT_CAPTION, alpha);
  if (e) {
    socialvis_status(e, status, sizeof status);
    afTexto(status, r.x, r.y + r.h + 42.0f, r.w, TXT_CAPTION2, alpha * 0.65f);
  }
}
static void afCartao(int i, GfxRect r, int foco, Uint32 agora) {
  int s = ajustes_amigos_estilo();
  const SvAmigo *a = afAmigo(i);
  const SvEvento *e = afEvento(i);
  afAnel(r, foco);
  if (s == 3 || (s == 1 && (!e || e->acao != SV_AGORA))) {
    char linha[256], ep[32];
    gfx_cor(r, 14.0f / r.h, 0.08f, 0.08f, 0.10f, 0.94f);
    svd_rosto((GfxRect){r.x + 16, r.y + (r.h - 48) * .5f, 48, 48}, a, 0, 1, agora);
    snprintf(linha, sizeof linha, "%s%s%s", a->nome, e ? " " : "", e ? socialvis_verbo(e) : "");
    afTexto(linha, r.x + 82, r.y + 12, r.w - 96, TXT_CAPTION, 1);
    if (e) {
      socialvis_ep(e, ep, sizeof ep);
      snprintf(linha, sizeof linha, "%s%s%s", e->titulo, ep[0] ? " · " : "", ep);
      afTexto(linha, r.x + 82, r.y + 42, r.w - 96, TXT_CAPTION2, .65f);
    }
    return;
  }
  svd_poster(r, e ? (s == 2 || s == 4 ? (e->poster[0] ? e->poster : e->arte) : (e->arte[0] ? e->arte : e->poster)) : "", 12.0f / r.h, 1);
  gfx_veu_base(r, 12.0f / r.h, .70f, .90f);
  if (e) {
    afTexto(e->titulo, r.x + 16, r.y + r.h - 42, r.w - 32, TXT_CALLOUT, 1);
    if (e->pct >= 0 && (e->acao == SV_AGORA || e->acao == SV_INICIO))
      svd_barra((GfxRect){r.x + 16, r.y + r.h - 10, r.w - 32, 4}, e->pct, 1);
    if (e->acao == SV_AGORA) {
      gfx_cor((GfxRect){r.x + r.w - 126, r.y + 16, 110, 32}, .5f, .65f, .08f, .16f, 1);
      afTexto(i18n("Agora"), r.x + r.w - 114, r.y + 18, 90, TXT_CAPTION2, 1);
    }
  }
  if (s == 4 && e) {
    int n = 0;
    for (int k = 0; k < socialvis_n_amigos() && n < 4; k++) {
      const SvAmigo *other = socialvis_amigo(k);
      for (int j = 0; j < other->nTit; j++) if (!strcmp(other->tit[j].imdb, e->imdb)) {
        svd_avatar((GfxRect){r.x + 14 + n * 30, r.y + 16, 40, 40}, other->avatar, other->nome, other->id, 1);
        n++; break;
      }
    }
  } else {
    svd_rosto((GfxRect){s == 2 ? r.x + (r.w - 48) * .5f : r.x + 16,
                             s == 2 ? r.y + r.h - 12 : r.y + 16, 48, 48}, a, 0, 1, agora);
  }
  if (s == 2) {
    afTexto(a->nome, r.x, r.y + r.h + 46, r.w, TXT_CAPTION, 1);
    if (e) afTexto(socialvis_verbo(e), r.x, r.y + r.h + 74, r.w, TXT_CAPTION2, .65f);
  } else afLegenda(i, r, 1);
}
static void afConvite(float x0, float y, int focada) {
  float w = amigosfilDireita() - x0;
  float iw = fminf(640.0f, w * .43f);
  float gap = 24.0f, ew = (w - iw - gap * 2.0f) * .5f;
  GfxRect r = {x0, y + 8, iw, NV_DESTAQUE_H};
  afAnel(r, focada);
  gfx_cor(r, 16.0f / r.h, .12f, .12f, .15f, .95f);
  float tx = x0 + 24, tw = iw - 48;
  if (celb_embutido(CELB_AMIGO)) {
    celb_desenhar_qr_em(CELB_AMIGO, (GfxRect){x0 + iw - 204, y + 32, 176, 176}, 1);
    tw -= 210;
  }
  float yy = y + 32;
  yy += txt_bloco(TXT_CALLOUT, i18n("Adicione amigos"), 245, 245, 250, tx, yy, tw, 32, 1, 2) + 12;
  txt_bloco(TXT_CAPTION, i18n("Veja o que eles estão assistindo e troquem recomendações."),
            190, 190, 200, tx, yy, tw, 28, 1, yy > y + 100 ? 2 : 3);
  afTexto(i18n("Adicionar pelo celular"), tx, y + 198, tw, TXT_CAPTION, 1);
  if (!celb_embutido(CELB_AMIGO))
    celb_botao(CELB_AMIGO, (GfxRect){x0 + iw - 76, y + 24, 48, 48}, focada, NULL, 0, 0, 1);
  afExemplos = 2;
  for (int i = 0; i < 2; i++) {
    GfxRect c = {x0 + iw + gap + i * (ew + gap), y + 8, ew, NV_DESTAQUE_H};
    gfx_cor(c, 12.0f / c.h, .12f, .13f, .16f, .35f);
    svd_avatar((GfxRect){c.x + 16, c.y + 16, 48, 48}, "", i ? "B" : "A", "example", .3f);
    afTexto(i18n("Exemplo"), c.x + 20, c.y + c.h - 44, c.w - 40, TXT_CALLOUT, .35f);
  }
}
static void afAtualizar(float dt, int focada, int *colunaP) {
  int changed = afRev != socialvis_revisao() && afEstilo == ajustes_amigos_estilo();
  afMontar();
  int c = colunaP ? *colunaP : 0;
  if (changed && afId[0] && colunaP) for (int i = 0; i < afN; i++) {
    const SvAmigo *a = afAmigo(i); const SvEvento *e = afEvento(i);
    if (!strcmp(a->id, afId) && !strcmp(e ? e->imdb : "", afImdb)) { c = i; break; }
  }
  int max = socialvis_n_amigos() ? afN + 1 : 0;
  if (c < 0) c = 0;
  if (c > max) c = max;
  if (colunaP) *colunaP = c;
  dentro = -1;
  if (focada && c < afN) {
    const SvAmigo *a = afAmigo(c); const SvEvento *e = afEvento(c);
    snprintf(afId, sizeof afId, "%s", a->id);
    snprintf(afImdb, sizeof afImdb, "%s", e ? e->imdb : "");
  } else if (focada) afId[0] = afImdb[0] = 0;
  if (ultX0 >= 0 && focada && c < afN) {
    GfxRect r = afRect(c, ultX0, 0);
    float alvo = scroll;
    float dir = amigosfilDireita() - 12;
    if (r.x - alvo < ultX0 + 8) alvo = r.x - ultX0 - 8;
    if (r.x + r.w - alvo > dir) alvo = r.x + r.w - dir;
    if (alvo < 0) alvo = 0;
    scroll = anim_mola2_reduzida(&vScroll, scroll, alvo, dt, NV_MOLA2_SCROLL, ajustes_animacoes_reduzidas());
  }
  if (!socialvis_n_amigos() && ajustes_amigos_estilo() == 0 && focada) {
    if (!afConviteEntrou && recomenda_ativo() && celb_disponivel())
      celb_abrir_embutido(CELB_AMIGO, i18n("Código do amigo"));
    afConviteEntrou = 1;
  } else {
    afConviteEntrou = 0;
    if (celb_embutido(CELB_AMIGO)) celb_fechar_dono(CELB_AMIGO);
  }
}
static void afDesenhar(float x0, float y, float corte, int focada, int coluna, Uint32 agora) {
  afMontar(); afFoco = (GfxRect){0}; afExemplos = 0;
  if (amigosfil_convite()) { afConvite(x0, y, focada); return; }
  float dir = amigosfilDireita();
  gfx_recorte(x0 - 12, fmaxf(corte, y - 4), dir - x0 + 24, NV_TELA_H - fmaxf(corte, y - 4));
  for (int i = 0; i < afN; i++) {
    GfxRect r = afRect(i, x0 - scroll, y);
    if (ajustes_amigos_estilo() == 1 && i >= afVivos() && (i - afVivos()) % 3 == 0)
      afTexto(i18n("Recentes"), r.x, y + 8, r.w, TXT_CAPTION2, .6f);
    if (r.x + r.w < x0 || r.x > dir) continue;
    afCartao(i, r, focada && i == coluna, agora);
#ifdef NV_TOUCH_UI
    alvoToque(r, corte, i, -1);
#endif
  }
  gfx_recorte(0, corte, NV_TELA_W, NV_TELA_H - corte);
  for (int i = afN; i < afN + 2; i++) {
    GfxRect r = afAcaoRect(i, y);
    afAnel(r, focada && coluna == i);
    gfx_cor(r, .5f, .14f, .14f, .17f, 1);
    const char *label = i == afN ? i18n("Ver todos") : i18n("+ Adicionar");
    TxtLinha t = txt_linha_corta(TXT_CAPTION, label, 240, 240, 245, 255, r.w - 24);
    txt_desenhar(t, r.x + (r.w - t.w) * .5f, r.y + (r.h - t.h) * .5f);
#ifdef NV_TOUCH_UI
    alvoToque(r, corte, i, -1);
#endif
  }
}
#ifdef NV_AMIGOSFIL_TESTE
#include <assert.h>
void amigosfil_teste_conferir(int w, int h) {
  if (ajustes_amigos_estilo() == 5) return;
  assert(afEstilo == ajustes_amigos_estilo());
  for (int i = 0; i < afN; i++) {
    GfxRect r = afRect(i, ultX0, 0);
    assert(r.w > 0 && r.w < w && r.h > 0 && r.h < h);
    if (afEstilo == 0) { assert(r.w == NV_DESTAQUE_W); assert(r.h == NV_DESTAQUE_H); }
  }
  assert(afFoco.w > 0 && afFoco.h > 0);
  assert(afFoco.x >= 0 && afFoco.y >= 0);
  assert(afFoco.x + afFoco.w <= w && afFoco.y + afFoco.h <= h);
  if (amigosfil_convite()) { assert(afExemplos == 2); assert(amigosfil_n_colunas() == 1); }
  for (int i = 0; i < afN; i++) {
    int count = 0;
    for (int j = 0; j < afN; j++) if (afItens[j].amigo == afItens[i].amigo) count++;
    assert(count <= 2);
    if (i && afEvento(i) && afEvento(i)->acao == SV_AGORA)
      assert(afEvento(i - 1) && afEvento(i - 1)->acao == SV_AGORA);
  }
}
#endif
