// Player externo (#501) — ver extplayer.h.
#include "extplayer.h"
#include <stddef.h>

int extplayer_decidir(int modo, const char *cabs, int *avisar) {
  if (avisar) *avisar = 0;
  if (modo != EXTPLAYER_EXTERNO && modo != EXTPLAYER_PERGUNTAR) return EXTPLAYER_INTERNO;
  if (cabs && cabs[0]) { if (avisar) *avisar = 1; return EXTPLAYER_INTERNO; }
  return modo;
}

#ifdef NV_ANDROID
#include "ajustes.h"
#include "android.h"
#include "idioma.h"
#include "ilha.h"
#include "player.h"
#include "video.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#define CHAVE_PERGUNTA "extplayer:onde"

static int perguntaAberta;
static Uint32 perguntaDesde;
static char pendUrl[2048], pendTitulo[160];
static void (*pendInterno)(const char *);
static void (*pendSair)(void);

static void abrirExterno(const char *url, const char *titulo,
                         void (*interno)(const char *), void (*sair)(void)) {
  if (android_abrir_externo(url, titulo)) {
    printf("[extplayer] aberto no player externo\n");
    sair();   // sem sincronia: o item fica como iniciado
    return;
  }
  printf("[extplayer] nenhum app abriu; player interno\n");
  player_toast(i18n("Player externo indisponível; usando o interno"), 3500u);
  interno(url);
}

int extplayer_tentar(const char *url, const char *titulo,
                     void (*interno)(const char *url), void (*sair)(void)) {
  int avisar, d;
  if (!url || !url[0] || perguntaAberta) return 0;
  d = extplayer_decidir(ajustes_player_externo(), video_cabecalhos(), &avisar);
  if (avisar) player_toast(i18n("Player externo indisponível; usando o interno"), 3500u);
  if (d == EXTPLAYER_INTERNO) return 0;
  if (d == EXTPLAYER_EXTERNO) { abrirExterno(url, titulo, interno, sair); return 1; }
  {
    static IlhaModal m;
    IlhaAvisoEx e;
    snprintf(pendUrl, sizeof pendUrl, "%s", url);
    snprintf(pendTitulo, sizeof pendTitulo, "%s", titulo ? titulo : "");
    pendInterno = interno; pendSair = sair;
    memset(&m, 0, sizeof m);
    snprintf(m.kicker, sizeof m.kicker, "%s", i18n("Player"));
    snprintf(m.titulo, sizeof m.titulo, "%s", i18n("Onde reproduzir?"));
    snprintf(m.icone, sizeof m.icone, "aj_circle-play");
    m.tipo = ILHA_ACENTO;
    m.nBotoes = 2;
    snprintf(m.botao[0], sizeof m.botao[0], "%s", i18n("Interno"));
    snprintf(m.botao[1], sizeof m.botao[1], "%s", i18n("Externo"));
    memset(&e, 0, sizeof e);
    e.chave = CHAVE_PERGUNTA; e.tipo = ILHA_ACENTO; e.icone = "aj_circle-play";
    e.texto = i18n("Onde reproduzir?"); e.ms = 60000u; e.modal = &m; e.abrir = 1;
    ilha_avisar_ex(&e);
    perguntaAberta = 1;
    perguntaDesde = SDL_GetTicks();
    return 1;
  }
}

static void concluir(int externo) {
  void (*interno)(const char *) = pendInterno;
  void (*sair)(void) = pendSair;
  perguntaAberta = 0;
  if (externo) abrirExterno(pendUrl, pendTitulo, interno, sair);
  else interno(pendUrl);
  pendUrl[0] = 0;
}

void extplayer_acao(const char *chave, int botao) {
  if (!chave || strcmp(chave, CHAVE_PERGUNTA) || !perguntaAberta) return;
  concluir(botao == 2);
}

void extplayer_passo(unsigned agora) {
  // VOLTAR: o modal recolheu sem botao. O padrao e o interno.
  if (perguntaAberta && agora - perguntaDesde > 600u && !ilha_tem(CHAVE_PERGUNTA) &&
      !ilha_modal_aberto() && !ilha_modal_visivel())
    concluir(0);
}
#endif
