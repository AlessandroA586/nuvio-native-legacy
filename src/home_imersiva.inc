// LAYOUT IMERSIVA (experimental). Incluido por home.c depois dos ajudantes de
// cartaz, logo e sinopse, que ele reusa. Pedido do dono: "nada de UI
// aparecendo, as fileiras e a arte, detalhe e animacao entre as fileiras, o
// filme abrindo dentro do proprio componente com cores".
//
// Tela: o fundo Imersivo do DESIGN.md §6 (a cor da arte em degrade pontilhado,
// GFX_FUNDO_DIN; no nivel de GPU cheio, a copia desfocada por cima), a arte
// nitida do titulo em foco no alto (a Cortina do Palco: some em ALFA para a
// esquerda e para baixo), logo + meta + sinopse como tipografia sobre a arte,
// UMA fileira em cena e as vizinhas como fantasmas nas bordas. Menu, ilha do
// relogio e dicas so com o foco no destaque (Cima na 1a fileira / Voltar) ou
// com o menu aberto (home_cromo_alfa, app.c). O estado mora em imersiva.c.
//
// GPU (gpunivel): efeitos leves ou minimos = so dissolve, sem desfoque, sem
// subida/escada, sem brilho tinto. O resto e o mesmo desenho.

#define IMV_X        96.0f    // margem esquerda (sem rail: o menu esta escondido)
// y em px da TELA (ANC): a home desenha num espaco escalado pelo texto.
#define IMV_LOGO_B   ANC(330.0f)   // base da caixa do logo
#define IMV_LOGO_W   640.0f
#define IMV_LOGO_H   190.0f
#define IMV_SIN_W    820.0f
#define IMV_TIT_Y    ANC(586.0f)   // titulo da fileira em cena
#define IMV_CARD_Y   ANC(640.0f)
#define IMV_CARD_H   330.0f
#define IMV_GAP      28.0f
#define IMV_SOBE     64.0f    // quanto a fileira sobe/desce na troca
#define IMV_FANT_H   190.0f   // cartaz dos fantasmas
#define IMV_FANT_A   0.26f
#define IMV_JAN      ((GfxRect){ IMV_X, ANC(556.0f), NV_TELA_W - 2.0f * IMV_X, ANC(470.0f) })
#define IMV_JAN_RAIO 28.0f    // NV_FOLHA_RAIO

static int   imvIni;
static float imvCor[3] = { 28 / 255.0f, 28 / 255.0f, 30 / 255.0f }, imvLuz[3] = { .3f, .3f, .32f };
static float imvCromo = 1.0f;
static float imvRola[MAX_FIL];
static int   imvItem = -1, imvItemAnt = -1;
static float imvArteT = 1.0f;          // dissolve da arte (e da cor) entre titulos

static int imvBaixo(void) { return gfx_efeitos_leves() || gfx_efeitos_minimos(); }
static int imvFileiraOk(int r) { return r >= 0 && r < nFileiras; }
static int imvNCol(int r) { return fileiras[r].n + (fileiras[r].verTudo ? 1 : 0); }

// O titulo em cena: o destaque com o foco nele, senao o cartaz focado.
static int imvItemCena(void) {
  if (focoHero) return ajustes_hero_ligado() ? heroAtual : -1;
  if (!imvFileiraOk(foco.fileira)) return -1;
  { int c = foco.coluna < fileiras[foco.fileira].n ? foco.coluna : fileiras[foco.fileira].n - 1;
    return fileiraItemIndice(&fileiras[foco.fileira], c); }
}

float home_cromo_alfa(void) { return ajustes_home_imersiva() ? imvCromo : 1.0f; }

static void imvAtualizar(float dt) {
  const int red = ajustes_animacoes_reduzidas();
  int alvoFil = imvFileiraOk(foco.fileira) ? foco.fileira : 0, it = imvItemCena();
  if (!imvIni) { imv_iniciar(&imv, alvoFil); imvIni = 1; }
  if (imv.aberto && (focoHero || foco.fileira != imv.fileira)) imv_fechar(&imv);
  imv_fileira(&imv, alvoFil);
  imv_passo(&imv, dt, red);
  imvCromo = anim_rampa(imvCromo, focoHero ? 1.0f : 0.0f, dt, 240.0f);
  if (it != imvItem) {
    // Dissolve interrompido no meio: a arte nova substitui a que chegava.
    if (imvArteT >= 0.5f) imvItemAnt = imvItem;
    imvItem = it; imvArteT = red || imvItemAnt < 0 ? 1.0f : 0.0f;
  }
  imvArteT = fminf(1.0f, imvArteT + dt * (1000.0f / 360.0f));
  for (int r = 0; r < nFileiras && r < MAX_FIL; r++) {
    float alvo = fmaxf(0.0f, (float)colunaDaFileira(r) - 2.0f);
    if (r == foco.fileira && foco.coluna >= fileiras[r].n) alvo = fmaxf(0.0f, (float)foco.coluna - 2.0f);
    imvRola[r] = red ? alvo : imvRola[r] + (alvo - imvRola[r]) * (1.0f - expf(-dt / 0.09f));
  }
  // A cor: o matiz da arte, escuro o bastante para o texto branco (o L do
  // Imersivo da Dinamica, com mais croma: aqui a cor e o assunto).
  { CorvivaPaleta p;
    const char *a = it >= 0 ? arte_por_identidade(it, 2) : NULL;
    if (a && corviva_paleta(a, &p) && p.ok) {
      float alvo[3], e = red ? 1.0f : 1.0f - expf(-dt / 0.18f);
      CorvivaTokens tk;
      dinTinge(p.acento, 0.20f, 0.27f, 0.6f, 0.07f, alvo);
      corviva_tokens(p.acento, &tk);
      for (int k = 0; k < 3; k++) {
        imvCor[k] += (alvo[k] - imvCor[k]) * e;
        imvLuz[k] += (tk.luz[k] - imvLuz[k]) * e;
      }
    }
  }
}

// A arte do titulo: desfocada em tela cheia (so GPU cheia) e a nitida no alto,
// ambas sumindo em alfa para a cor do fundo.
static void imvArte(int idx, float a) {
  const char *art = idx >= 0 ? arte_por_identidade(idx, 2) : NULL;
  GLuint t = art ? tex_obter_hero(art) : 0;
  if (!t || a <= 0.004f) return;
  gfx_tex_aspect_atual = tex_aspecto(art);
  if (!imvBaixo()) {
    GLuint b = gfx_desfocado(t, art);
    if (b) gfx_rect((GfxRect){ 0, 0, NV_TELA_W, NV_TELA_H }, b, GFX_PALCO_ARTE, 0, 1, 0.5f, 0, 0, 0, 0, a * 0.7f);
  }
  gfx_rect((GfxRect){ NV_TELA_W * 0.20f, 0, NV_TELA_W * 0.80f, NV_TELA_H * 0.80f }, t,
           GFX_PALCO_ARTE, 0, 0, 0.5f, 0, 0, 0, 0, a);
  gfx_tex_aspect_atual = 0;
}

// Logo (ou o nome), uma linha de meta e a sinopse curta, sobre a arte.
static void imvTexto(const CatItem *ci, float x, float logoB, float logoW, float logoH,
                     float sinW, int linhas, float a) {
  if (!ci || a <= 0.004f) return;
  const float og = gfx_opacidade_grupo;
  gfx_opacidade_grupo = og * a;
  const char *uL = artehero_logo_sessao_larg(ci, logoW);
  GLuint tl = uL ? tex_obter_logo_larg(uL, logoW) : 0;
  float asp = tl ? tex_aspecto(uL) : 0.0f;
  if (tl && asp > 0.01f) {
    float w = logoW, h = w / asp;
    if (h > logoH) { h = logoH; w = h * asp; }
    gfx_rect((GfxRect){ x, logoB - h, w, h }, tl, tex_marca_escura(uL) ? GFX_MARCA : GFX_TEXTO,
             0, 0, 0, 0, 1, 1, 1, 1);
  } else if (ci->titulo[0]) {
    TxtLinha t = txt_linha_corta(TXT_ILHA_TITULO, ci->titulo, 243, 242, 239, 255, logoW);
    txt_desenhar(t, x, logoB - t.h);
  }
  float y = logoB + 18.0f;
  y += heroiMeta(ci, x, y, sinW) + 14.0f;
  if (ci->sinopse[0] && linhas > 0)
    sinopse_bloco_corta(TXT_DET_SIN, ci->sinopse, 243, 242, 239, x, y, sinW, 36.0f, 0.92f, linhas);
  gfx_opacidade_grupo = og;
}

// Uma fileira de cartazes. modo 0 = em cena, 1 = saindo, 2 = fantasma.
static void imvCartazes(int r, float y, float h, float a, int modo, float sobe) {
  if (!imvFileiraOk(r) || a <= 0.004f) return;
  const Fileira *f = &fileiras[r];
  const float w = h * 2.0f / 3.0f, passo = w + IMV_GAP * h / IMV_CARD_H;
  const float rola = r < MAX_FIL ? imvRola[r] : 0.0f;
  const int baixo = imvBaixo(), n = imvNCol(r);
  int c0 = (int)floorf(rola) - 1;
  if (c0 < 0) c0 = 0;
  for (int c = c0; c < n && c < MAX_CARDS; c++) {
    float px = IMV_X + (c - rola) * passo;
    if (px > NV_TELA_W) break;
    float ca = a, dy = sobe, f1 = modo == 0 && r < MAX_FIL ? animFoco[r][c] : 0.0f;
    if (modo == 0 && imv_em_troca(&imv) && r == imv.fileira) {
      float e = imv_escada(&imv, c - (int)floorf(rola));
      ca *= baixo ? imv_chega(&imv) : e;
      if (!baixo) dy += (1.0f - e) * IMV_SOBE * (float)imv.dir;
    }
    // O cartaz aberto mora na janela (imvJanela), nao na fileira.
    if (modo == 0 && imv.abre > 0.0f && c == imv.coluna) continue;
    if (modo == 0 && imv.abre > 0.0f) ca *= 1.0f - imv_suave(imv.abre);
    if (ca <= 0.004f) continue;
    const float esc = 1.0f + 0.08f * f1;
    GfxRect card = { px - w * (esc - 1.0f) * 0.5f, y + dy - h * (esc - 1.0f) * 0.5f, w * esc, h * esc };
    const float raio = raioDe(card.w, card.h);
    gfx_opacidade_grupo = ca;
    if (c >= f->n) {   // "Ver tudo": a ilha do cartaz, sem arte
      gfx_vidro_ou_solido(card, raio, GFX_VIDRO_DENS_MODAL, 1.0f, 0.071f, 0.075f, 0.086f, 0.98f);
      TxtLinha t = txt_linha_corta(TXT_ILHA_NOME, i18n("Ver tudo"), 243, 242, 239, 255, card.w - 24.0f);
      txt_desenhar(t, card.x + (card.w - t.w) * 0.5f, card.y + (card.h - t.h) * 0.5f);
    } else {
      int idx = fileiraItemIndice(f, c);
      const char *cam = arte_por_identidade(idx, 0);
      GLuint t = cam ? tex_obter_larg(cam, w * 1.08f) : 0;
      if (f1 > 0.01f) {
        // Foco: escala ~1.08 e sombra curta (DESIGN §3); o brilho leva o matiz
        // da arte (imersivo-acentos: so a luz, nunca o acento cheio).
        gfx_rect((GfxRect){ card.x - 14, card.y + 10, card.w + 28, card.h + 28 }, 0, GFX_SOMBRA,
                 1.0f, 0, 0, 0.5f, 0, 0, 0, 0.45f * f1);
        if (!baixo)
          gfx_rect((GfxRect){ card.x - 60, card.y + card.h * 0.55f, card.w + 120, card.h * 0.6f }, 0,
                   GFX_SOMBRA, 1.0f, 0, 0, 0.5f, imvLuz[0], imvLuz[1], imvLuz[2], 0.55f * f1);
      }
      gfx_veu_card_limpar();
      desenhaArteCard(card, FILEIRA_POSTER, cam, t, cat_item_exato(idx), f1, raio, 1.0f, 0.0f);
      if (modo == 0 && r == foco.fileira && c == foco.coluna && !focoHero) {
        const CatItem *ci = cat_item_exato(idx);
        itemFoco = (HomeItem){ .indice = idx, .rect = card, .arte = cam,
          .titulo = ci ? ci->titulo : NULL, .genero = ci ? ci->genero : NULL,
          .meta = ci ? ci->meta : NULL };
        publicarItemFoco();
        ancoraFoco = card; temAncoraFoco = 1;
      }
    }
  }
  gfx_opacidade_grupo = 1.0f;
}

// Titulo da fileira + contador, como o Palco aprovado (stage-B).
static void imvTitulo(int r, float y, float a) {
  if (!imvFileiraOk(r) || a <= 0.004f) return;
  gfx_opacidade_grupo = a;
  TxtLinha tl = txt_linha_corta(TXT_ROW_TITULO, fileiras[r].titulo, 243, 242, 239, 255, 1100.0f);
  txt_desenhar(tl, IMV_X, y);
  if (r == foco.fileira && !focoHero) {
    char cur[16], tot[24];
    snprintf(cur, sizeof cur, "%d", (foco.coluna < fileiras[r].n ? foco.coluna : fileiras[r].n - 1) + 1);
    snprintf(tot, sizeof tot, " / %d", fileiras[r].n);
    TxtLinha n = txt_linha(TXT_ROW_TITULO, cur, 243, 242, 239, 255);
    TxtLinha t = txt_linha(TXT_CW_META, tot, 137, 140, 147, 255);
    float x = NV_TELA_W - IMV_X - n.w - t.w;
    txt_desenhar(n, x, y + (tl.h - n.h) * .5f);
    txt_desenhar(t, x + n.w, y + (tl.h - t.h) * .5f);
  }
  gfx_opacidade_grupo = 1.0f;
}

// OK num cartaz: ele vira uma janela larga NA PROPRIA fileira, com a arte 16:9
// de fundo, a cor do titulo, o detalhe e (serie) os episodios. Nivel de GPU
// baixo: o cartaz e a janela so se dissolvem, sem morph.
static void imvJanela(void) {
  if (imv.abre <= 0.0f || !imvFileiraOk(imv.fileira)) return;
  const Fileira *f = &fileiras[imv.fileira];
  if (imv.coluna >= f->n) return;
  const int idx = fileiraItemIndice(f, imv.coluna), baixo = imvBaixo();
  const CatItem *ci = cat_item_exato(idx);
  const float e = imv_suave(imv.abre);
  const float h = IMV_CARD_H * 1.08f, w = h * 2.0f / 3.0f;
  GfxRect cartaz = { IMV_X + (imv.coluna - imvRola[imv.fileira]) * (IMV_CARD_H * 2.0f / 3.0f + IMV_GAP)
                     - IMV_CARD_H * 2.0f / 3.0f * 0.04f, IMV_CARD_Y - IMV_CARD_H * 0.04f, w, h };
  GfxRect jan = IMV_JAN, r = baixo ? jan : (GfxRect){
    cartaz.x + (jan.x - cartaz.x) * e, cartaz.y + (jan.y - cartaz.y) * e,
    cartaz.w + (jan.w - cartaz.w) * e, cartaz.h + (jan.h - cartaz.h) * e };
  const char *poster = arte_por_identidade(idx, 0);
  const char *art = arte_por_identidade(idx, 2);
  GLuint tp = poster ? tex_obter_larg(poster, w) : 0, ta = art ? tex_obter_hero(art) : 0;
  const float aj = baixo ? e : fminf(1.0f, e * 1.6f);
  const float raio = IMV_JAN_RAIO / r.h;
  gfx_rect((GfxRect){ r.x - 14, r.y + 10, r.w + 28, r.h + 28 }, 0, GFX_SOMBRA, 1.0f, 0, 0,
           IMV_JAN_RAIO / (r.h + 28), 0, 0, 0, 0.5f);
  // O cartaz sai: no morph ele estica junto da janela ate sumir.
  if (aj < 1.0f) {
    gfx_veu_card_limpar();
    desenhaArteCard(baixo ? cartaz : r, FILEIRA_POSTER, poster, tp, ci, 1.0f,
                    raioDe(cartaz.w, cartaz.h), 1.0f - aj, 0.0f);
  }
  // A janela: a cor do titulo, a arte 16:9 por cima (veu de leitura a
  // esquerda) e a luz do matiz lavando tudo — nada de contorno.
  gfx_cor(r, raio, imvCor[0], imvCor[1], imvCor[2], aj);
  if (ta) {
    gfx_tex_aspect_atual = tex_aspecto(art);
    gfx_rect(r, ta, GFX_VITRINE, 0.86f, 0.3f, 0.0f, raio, 0.5f, 0, 0, aj);
    gfx_tex_aspect_atual = 0;
  }
  gfx_cor(r, raio, imvLuz[0], imvLuz[1], imvLuz[2], 0.22f * aj);
  if (e < 0.6f || !ci) return;
  const float at = (e - 0.6f) / 0.4f, x = r.x + 48.0f, tw = r.w * 0.46f;
  const int nEp = !strcmp(ci->tipo, "series") ? cat_n_episodios(idx) : 0;
  imvTexto(ci, x, r.y + 150.0f, 460.0f, 110.0f, tw, nEp > 0 ? 2 : 4, at);
  if (nEp <= 0) return;
  // Episodios: a partir do que esta em andamento, ate quatro.
  int ini = 0;
  for (int i = 0; i < nEp; i++) {
    const CatEp *ep = cat_episodio(idx, i);
    if (ep && ep->temporada == ci->temporada && ep->episodio == ci->episodio) { ini = i; break; }
  }
  const float ew = 200.0f, eh = 112.0f, ey = r.y + r.h - eh - 40.0f;
  gfx_opacidade_grupo = at;
  for (int i = 0; i < 4 && ini + i < nEp; i++) {
    const CatEp *ep = cat_episodio(idx, ini + i);
    if (!ep) break;
    GfxRect er = { x + i * (ew + 16.0f), ey, ew, eh };
    const char *th = ep->thumb[0] ? ep->thumb : art;
    GLuint t = th ? tex_obter_larg(th, ew) : 0;
    if (t) { gfx_tex_aspect_atual = tex_aspecto(th);
             gfx_rect(er, t, GFX_VITRINE, 0.7f, 0.5f, 0.0f, 16.0f / eh, 0.5f, 0, 0, 1.0f);
             gfx_tex_aspect_atual = 0; }
    else gfx_cor(er, 16.0f / eh, 1, 1, 1, 0.08f);
    char rot[160];
    snprintf(rot, sizeof rot, "E%d · %s", ep->episodio, ep->nome);
    TxtLinha tl = txt_linha_corta(TXT_ILHA_HORA, rot, 243, 242, 239, 255, ew - 20.0f);
    txt_desenhar(tl, er.x + 10.0f, er.y + er.h - tl.h - 8.0f);
  }
  gfx_opacidade_grupo = 1.0f;
}

static void imvDesenhar(Uint32 agora) {
  (void)agora;
  const float pd = detail_progresso();
  const int baixo = imvBaixo();
  // Fundo: a cor do titulo (opaco, pontilhado) e as artes em dissolve.
  gfx_fundo_din_desenhar(imvCor, 0.55f);
  // A janela aberta e o assunto: a arte de tras recua para 35%.
  { const float e = imv_suave(imvArteT), recua = 1.0f - 0.65f * imv_suave(imv.abre);
    if (e < 1.0f) imvArte(imvItemAnt, (1.0f - e) * recua);
    imvArte(imvItem, e * recua); }
  { const char *a = imvItem >= 0 ? arte_por_identidade(imvItem, 2) : NULL;
    if (a) corviva_definir(a, CORVIVA_HOME); }
  if (pd >= 0.996f) return;
  const float casa = 1.0f - pd;
  const float aberto = imv_suave(imv.abre);
  // Detalhe do titulo em cena: some quando a janela abre (ela tem o seu).
  { const float e = imv_suave(imvArteT);
    imvTexto(cat_item_exato(imvItem), IMV_X, IMV_LOGO_B, IMV_LOGO_W, IMV_LOGO_H, IMV_SIN_W, 3,
             casa * (1.0f - aberto) * (0.35f + 0.65f * e)); }
  // Fantasmas: a de cima encostada no topo, a de baixo na base.
  { const float fa = IMV_FANT_A * casa * (1.0f - aberto) * imv_chega(&imv);
    imvCartazes(imv_fantasma(&imv, nFileiras, -1), -IMV_FANT_H + 46.0f, IMV_FANT_H, fa, 2, 0);
    int prox = imv_fantasma(&imv, nFileiras, 1);
    imvTitulo(prox, NV_TELA_H - ANC(70.0f), fa * 1.4f);
    imvCartazes(prox, NV_TELA_H - ANC(28.0f), IMV_FANT_H, fa, 2, 0); }
  // A que sai sobe (descendo o foco) ou desce, apagando; a que entra sobe em escada.
  if (imv.anterior >= 0) {
    // Sai mais depressa do que a outra entra (s^2): no meio da troca as duas
    // fileiras nao disputam a mesma faixa.
    const float s = imv_sai(&imv), dy = baixo ? 0.0f : -(1.0f - s) * IMV_SOBE * 1.6f * (float)imv.dir;
    imvTitulo(imv.anterior, IMV_TIT_Y + dy, s * s * casa);
    imvCartazes(imv.anterior, IMV_CARD_Y, IMV_CARD_H, s * s * casa, 1, dy);
  }
  { const float c = imv_chega(&imv), dy = baixo ? 0.0f : (1.0f - c) * IMV_SOBE * (float)imv.dir;
    imvTitulo(imv.fileira, IMV_TIT_Y + dy, c * casa * (1.0f - aberto));
    imvCartazes(imv.fileira, IMV_CARD_Y, IMV_CARD_H, casa, 0, 0); }
  gfx_opacidade_grupo = casa;
  imvJanela();
  gfx_opacidade_grupo = 1.0f;
}

// OK curto num cartaz abre no lugar; com a janela aberta, OK segue para a
// pagina do titulo (ou toca, na retomada) e Voltar fecha. 1 = consumido.
static int imvEvento(const SDL_Event *e) {
  if (!ajustes_home_imersiva() || !imv.aberto) return 0;
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0;
  SDL_Keycode k = e->key.keysym.sym;
  if (k == SDLK_ESCAPE || k == SDLK_AC_BACK || k == SDLK_BACKSPACE) {
    if (e->type == SDL_KEYDOWN) imv_fechar(&imv);
    return 1;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) return 0;   // o caminho do OK, abaixo
  return 1;   // setas com a janela aberta: nada (sem fila)
}
