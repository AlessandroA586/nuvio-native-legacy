// APOIAR O PROJETO (Sobre e ajuda): o painel com os QRs de verdade (apoio.h).
// Era a previa da linha na 2.0.2; no redesenho a previa virou uma cena sem
// palavras (ajcT_apoiar) e os codigos sumiram, entao o OK da linha abre este
// painel. Mesmos enderecos de sempre (apoio.c). O QR vai num cartao branco de
// cantos arredondados (apoio_qr), grande o bastante para a camera a 3 m.
// OK e Voltar fecham e a pessoa continua na linha "Apoiar o projeto".
static void apoioAbrir(void) { apoioAberto = 1; }
static void apoioFechar(void) { apoioAberto = 0; focarOpcao(AJ_APOIAR); }
static void apoioEvento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!e || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE || guiaVoltar(k, e)) apoioFechar();
}
static void apoioDesenhar(void) {
  const float lado = 300.0f, gap = 100.0f, pad = 44.0f;
  int n = apoio_n(), i;
  float total = n > 0 ? (float)n * lado + (float)(n - 1) * gap : 0.0f;
  float w = total + 2 * pad + 120.0f, h, y, x0;
  GfxRect m;
  AjDica d[1] = { { "Voltar", "Fechar" } };
  float dw = ajDicasLargura(d, 1);
  if (w < 760.0f) w = 760.0f;
  h = 84.0f + 80.0f + 34.0f + lado + 22.0f + 64.0f + 12.0f + 30.0f + pad;
  m = (GfxRect){ (NV_VTELA_W - w) * 0.5f, (NV_VTELA_H - h) * 0.5f, w, h };
  ajVeuModal();
  ajIlha(m, 36, 1, 1);
  ajIcone("aj_heart", m.x + pad, m.y + 36, 22, 0.8f);
  ajKicker("Apoiar o projeto", m.x + pad + 34, m.y + 38, 1);
  ajDicasLinha(d, 1, m.x + m.w - pad - dw, m.y + 40, 1);
  y = m.y + 84;
  y += ajBloco(TXT_AJ_SUB, ajudaOpcao(AJ_APOIAR), m.x + pad, y, m.w - 2 * pad, 28.5f, 0.62f, 3);
  y += 34;
  x0 = m.x + (m.w - total) * 0.5f;
  for (i = 0; i < n; i++) {
    int q = apoio_qual(i);
    float qx = x0 + (float)i * (lado + gap);
    TxtLinha u = txt_linha_corta(TXT_V2_18, apoio_url_curta(q), 243, 242, 239, 255, lado + gap * 0.8f);
    apoio_qr(q, qx, y, lado, 1.0f);
    apoio_rotulo(q, qx + lado * 0.5f, y + lado + 22, 64.0f, 1, 1.0f);
    txt_desenhar_alpha(u, qx + (lado - (float)u.w) * 0.5f, y + lado + 22 + 64 + 12, 0.62f);
  }
}
