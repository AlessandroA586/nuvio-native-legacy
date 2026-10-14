// Four small, verified tips. No artwork, network or additional animation engine.
#include "dicascontrole.h"
#include "ajustes.h"
#include "anim.h"
#include "botoes.h"
#include "dados.h"
#include "gfx.h"
#include "idioma.h"
#include "layout.h"
#include "ponteiro.h"
#include "telefoneui.h"
#include "text.h"
#include <stdlib.h>

static int aberto, pagina, modoTeste = -1, fundoPronto;
static float entrada, troca = 1, tempo, espera;
static GfxMini fundo;

int dicascontrole_modo(void) {
  if (modoTeste >= 0) return modoTeste;
  if (telefoneui_ativo()) return DICAS_TOQUE;
#if defined(NV_WEBOS)
  return DICAS_TV_LG;
#elif defined(__EMSCRIPTEN__) || defined(NV_TPK40)
  return DICAS_TV_WGT;
#else
  return DICAS_TV_CH;
#endif
}
void dicascontrole_teste_modo(int modo) { modoTeste = modo; }
int dicascontrole_aberto(void) { return aberto; }
int dicascontrole_visivel(void) { return entrada > .002f || aberto; }
int dicascontrole_ocupado(void) { return aberto || entrada > .002f || espera > 0; }
int dicascontrole_pagina(void) { return pagina; }

void dicascontrole_abrir(void) {
  aberto = 1; pagina = 0; entrada = 0; troca = 1; tempo = espera = 0;
  fundoPronto = 0;
}
void dicascontrole_depois_novidades(void) {
  if (dicascontrole_ocupado()) return;
  char *visto = dados_ler(DICASCONTROLE_ARQUIVO);
  if (visto) { free(visto); return; }
  espera = .36f; // Let the outgoing What's New card finish its fade first.
}
static void concluir(int a, int b) {
  (void)a; (void)b;
  if (!aberto) return;
  aberto = 0;
  dados_gravar(DICASCONTROLE_ARQUIVO, "1\n");
  ponteiro_cancelar_toque();
}
static void ir(int p, int b) {
  (void)b;
  if (!aberto || p < 0 || p > 3 || p == pagina) return;
  pagina = p; troca = 0;
}
void dicascontrole_evento(const SDL_Event *e) {
  if (!aberto || e->type != SDL_KEYDOWN || e->key.repeat) return;
  SDL_Keycode k = e->key.keysym.sym;
  if (k == SDLK_LEFT) ir(pagina - 1, 0);
  else if (k == SDLK_RIGHT) ir(pagina + 1, 0);
  else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE ||
           k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE ||
           k == SDLK_DELETE || e->key.keysym.scancode == NV_SCANCODE_BACK) concluir(0, 0);
}
void dicascontrole_atualizar(float dt, Uint32 agora) {
  (void)agora;
  if (espera > 0) {
    espera -= dt;
    if (espera <= 0) dicascontrole_abrir();
  }
  int red = ajustes_animacoes_reduzidas();
  entrada = red ? (float)aberto : anim_rampa(entrada, (float)aberto, dt, aberto ? 320 : 180);
  troca = red ? 1 : anim_rampa(troca, 1, dt, 260);
  if (aberto) tempo += dt;
}
const char *dicascontrole_tecla(int p) {
  int m = dicascontrole_modo();
  if (m == DICAS_TOQUE) {
    static const char *const toque[] = { "Relógio na tela", "Buscar", "Segure para opções", "Barra lateral" };
    return toque[p];
  }
  if (p == 0) return "CH+";
  if (p == 1) return m == DICAS_TV_CH ? "CH-" : "Amarelo";
  return p == 2 ? "Segurar CH+" : "Segure OK";
}
static const char *titulo(int p) {
  static const char *const tv[] = { "Painel de Salvos", "Spotlight", "Central de controle", "Segurar OK num cartaz" };
  static const char *const toque[] = { "Painel de Salvos", "Spotlight", "Menu do cartaz (segurar OK)", "Barra lateral" };
  return dicascontrole_modo() == DICAS_TOQUE ? toque[p] : tv[p];
}
static const char *frase(int p) {
  if (dicascontrole_modo() == DICAS_TOQUE) {
    static const char *const toque[] = {
      "Toque no relógio para abrir Salvos e Avisos.",
      "Segure Buscar no menu para abrir o Spotlight.",
      "Toque e segure um cartaz para abrir o menu.",
      "Deslize da borda esquerda para abrir o menu."
    };
    return toque[p];
  }
  static const char *const tv[] = {
    "CH+ abre Salvos e Avisos na lateral direita.",
    "Buscar filmes, séries, pessoas e canais",
    "Segure CH+ e a ilha vira um painel de ajustes rápidos, com atalhos que você escolhe.",
    "Segure OK para opções"
  };
  return tv[p];
}

void dicascontrole_fundo(void (*desenhar)(Uint32), Uint32 agora) {
  int w = 96, h = (int)(96 * NV_TELA_H / NV_TELA_W);
  if (!fundoPronto || fundo.w != w || fundo.h != h) {
    if (!gfx_snap_ok() || !gfx_mini_alvo(&fundo, w, h)) { desenhar(agora); return; }
    // Settings help can itself draw miniatures: capture before downsampling,
    // since gfx_mini does not allow nested rendering targets.
    gfx_snap_comecar();
    desenhar(agora);
    gfx_snap_terminar();
    gfx_mini_comecar(&fundo, 0, 0, 96 / NV_TELA_W);
    gfx_snap_desenhar();
    gfx_mini_terminar();
    fundoPronto = 1;
  }
  gfx_mini_desenhar(&fundo, (GfxRect){0, 0, NV_TELA_W, NV_TELA_H}, 0, 1);
}
static void texto(const char *s, TxtEstilo estilo, float x, float y, float w, float a) {
  TxtLinha l = txt_linha_corta(estilo, i18n(s), 243, 242, 239, 255, w);
  txt_desenhar_alpha(l, x + (w - l.w) * .5f, y, a);
}
static void chave(GfxRect r, const char *s, int on, float a) {
  float ar, ag, ab;
  ajustes_acento(&ar, &ag, &ab);
  float pulse = ajustes_animacoes_reduzidas() ? 0 : .5f + .5f * sinf(tempo * 3);
  if (on) {
    GfxRect halo = {r.x - 7, r.y - 7, r.w + 14, r.h + 14};
    gfx_anel(halo, .45f, 2, ar, ag, ab, (.3f + .3f * pulse) * a);
  }
  gfx_cor(r, .45f, on ? ar : .16f, on ? ag : .17f, on ? ab : .20f, a);
  TxtLinha l = txt_linha_corta(TXT_CAPTION, i18n(s), on ? ajustes_tinta_foco() : 243,
                             on ? ajustes_tinta_foco() : 242, on ? ajustes_tinta_foco() : 239, 255, r.w - 10);
  txt_desenhar_alpha(l, r.x + (r.w - l.w) * .5f, r.y + (r.h - l.h) * .5f, a);
}
static void remoto(float cx, float y, float a) {
  GfxRect r = {cx - 116, y, 232, 500};
  gfx_cor(r, .14f, .28f, .29f, .32f, a);
  gfx_anel(r, .14f, 1.5f, 1, 1, 1, .19f * a);
  gfx_cor((GfxRect){r.x + 5, y + 5, r.w - 10, r.h - 10}, .13f, .075f, .08f, .10f, a);
  gfx_cor((GfxRect){cx - 64, y + 55, 128, 128}, .5f, .19f, .20f, .23f, a);
  texto("↑", TXT_CAPTION, cx - 28, y + 62, 56, .6f * a);
  texto("↓", TXT_CAPTION, cx - 28, y + 149, 56, .6f * a);
  texto("‹", TXT_CAPTION, cx - 60, y + 101, 36, .6f * a);
  texto("›", TXT_CAPTION, cx + 24, y + 101, 36, .6f * a);
  chave((GfxRect){cx - 36, y + 91, 72, 58}, "OK", pagina == 3, a);
  chave((GfxRect){cx - 80, y + 220, 160, 60}, "CH+", pagina == 0 || pagina == 2, a);
  chave((GfxRect){cx - 80, y + 304, 160, 60}, "CH-", pagina == 1 && dicascontrole_modo() == DICAS_TV_CH, a);
  chave((GfxRect){cx - 80, y + 405, 160, 44}, "Amarelo", pagina == 1 && dicascontrole_modo() != DICAS_TV_CH, a);
}
static void celular(float cx, float y, float a) {
  GfxRect r = {cx - 135, y, 270, 440};
  gfx_cor(r, .10f, .27f, .28f, .31f, a);
  gfx_cor((GfxRect){r.x + 7, r.y + 7, r.w - 14, r.h - 14}, .085f, .07f, .08f, .11f, a);
  gfx_cor((GfxRect){cx - 42, y + 15, 84, 15}, .5f, 0, 0, 0, a);
  chave((GfxRect){cx + 2, y + 52, 110, 46}, "12:34", pagina == 0, a);
  chave((GfxRect){cx - 112, y + 112, 224, 52}, "Buscar", pagina == 1, a);
  chave((GfxRect){cx - 62, y + 197, 124, 160}, "N", pagina == 2, a);
  if (pagina == 3) chave((GfxRect){cx - 126, y + 238, 70, 60}, "→", 1, a);
  gfx_cor((GfxRect){cx - 48, y + 418, 96, 5}, .5f, 1, 1, 1, .5f * a);
}
static int deslizar(const PonteiroRolagem *e) {
  static float distancia;
  if (e->fase == PONT_ROL_INICIO) { distancia = 0; return aberto; }
  if (e->fase == PONT_ROL_MOVER && !e->eixoY) distancia += e->delta;
  if (e->fase == PONT_ROL_SOLTAR && !e->eixoY && fabsf(distancia) > 50)
    ir(pagina + (distancia < 0 ? 1 : -1), 0);
  return 1;
}
void dicascontrole_desenhar(Uint32 agora) {
  (void)agora;
  if (entrada <= .002f) return;
  int toque = dicascontrole_modo() == DICAS_TOQUE;
  int vertical = toque && NV_TELA_H > NV_TELA_W;
  float a = anim_suave(entrada), t = anim_saida(troca);
  float w = fminf(1440, NV_TELA_W - 96), h = vertical ? fminf(1100, NV_TELA_H - 96) : 860;
  h = fminf(h, NV_TELA_H - 64);
  GfxRect r = {(NV_TELA_W - w) * .5f, (NV_TELA_H - h) * .5f + (1 - a) * 28, w, h};
  gfx_sem_recorte();
  gfx_cor((GfxRect){0, 0, NV_TELA_W, NV_TELA_H}, 0, 0, 0, 0, .65f * a);
  if (ajustes_vidro()) gfx_vidro_folha(r, 36 / r.h, a);
  else gfx_cor(r, 36 / r.h, .055f, .06f, .075f, .97f * a);
  gfx_anel(r, 36 / r.h, 1.2f, 1, 1, 1, .12f * a);
  if (aberto) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, NULL, 0, 0);
    if (toque) ponteiro_rolagem(deslizar);
  }
  texto("Dicas do controle", TXT_ILHA_TITULO, r.x + 40, r.y + 38, r.w - 80, a);
  float cx = vertical ? r.x + r.w * .5f : r.x + r.w * .26f;
  float iy = r.y + (vertical ? 115 : 174);
  if (toque) celular(cx, iy, a * t); else remoto(cx, iy, a * t);
  float tx = vertical ? r.x + 48 : r.x + r.w * .47f;
  float tw = vertical ? r.w - 96 : r.w * .53f - 64;
  float ty = vertical ? iy + 465 : r.y + 290;
  ty += (1 - t) * 16;
  texto(dicascontrole_tecla(pagina), TXT_CALLOUT, tx, ty, tw, .65f * a * t);
  const char *nome = toque && pagina == 2 ? "Segure para opções" : titulo(pagina);
  float th = txt_bloco_corta(TXT_ILHA_TITULO, i18n(nome), 243, 242, 239, tx, ty + 52, tw, 48, a * t, 0);
  txt_bloco_corta(TXT_BODY, i18n(frase(pagina)), 198, 202, 212, tx, ty + 72 + th, tw, 38, .9f * a * t, 0);
  if (!toque && pagina < 3) texto("Na Home", TXT_CAPTION, tx, r.y + r.h - 222, tw, .48f * a);
  float dy = r.y + r.h - 155;
  for (int i = 0; i < 4; i++) {
    GfxRect dot = {r.x + r.w * .5f - 66 + i * 36, dy, 12, 12};
    gfx_cor(dot, .5f, 1, 1, 1, (i == pagina ? .95f : .25f) * a);
    if (aberto) ponteiro_alvo(dot.x - 16, dot.y - 16, 44, 44, NULL, ir, i, 0);
  }
  GfxRect fim = {r.x + r.w * .5f - 130, r.y + r.h - 110, 260, 64};
  botao_pilula(fim, "Concluir", NULL, 1, 1, 0, a);
  if (aberto) ponteiro_alvo(fim.x, fim.y, fim.w, fim.h, NULL, concluir, 0, 0);
  if (!toque) texto("← → páginas · OK ou Voltar conclui", TXT_CAPTION, r.x + 40, r.y + r.h - 38, r.w - 80, .5f * a);
}
void dicascontrole_encerrar(void) { gfx_mini_liberar(&fundo); }
