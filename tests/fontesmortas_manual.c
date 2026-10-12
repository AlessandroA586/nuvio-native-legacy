// Recusa sincrona: video_tocar=0 -> player real em erro -> vigia real -> disco.
#define SDL_MAIN_HANDLED
#include "../src/app.c"
#include <assert.h>
#include "dados.h"
#include "linguas.h"

static int backendFalhou, chamadas;
int video_tocar(const char *url) {
  assert(!strcmp(url, "https://example.invalid/recusado.mp4"));
  chamadas++; backendFalhou = 1; return 0;
}
int video_falhou(void) { return backendFalhou; }
int video_pronto(void) { return 0; }
double video_pos(void) { return 0; }

int main(void) {
  CatItem c = {0};
  Stream *s = calloc(1, sizeof *s); assert(s);
  assert(!SDL_Init(SDL_INIT_TIMER)); dados_iniciar(""); ling_local_legenda("none");
  strcpy(c.titulo, "Fixture"); strcpy(c.tipo, "movie"); cat_definir(&c, 1);
  strcpy(s->provedor, "Addon"); strcpy(s->url, "https://example.invalid/recusado.mp4");
  stream_definir_lista(s, 1); stream_definir_atual(0);
  player_abrir(0, NULL);
  player_definir_fonte(s->url);
  SDL_Delay(1); fonteManualDesde = SDL_GetTicks();
  assert(chamadas == 1 && video_falhou() && player_fonte_falhou());
  assert(stream_vod_falhou_ha(0) < 0);
  vigiarAberturaManual();
  assert(!fonteManualDesde);
  assert(stream_vod_falhou_ha(0) >= 0 && "recusa sincrona manual escapou da marcacao");
  fontesmortas_iniciar();
  // Outra busca, token renovado e indice novo ainda encontram a falha no disco.
  strcat(s->url, "?token=novo"); stream_definir_lista(s, 1);
  assert(stream_vod_falhou_ha(0) >= 0 && !stream_automatico_disponivel(0));
  stream_definir_lista(NULL, 0); free(s);
  puts("fontesmortas: PASS (video_tocar=0, player e vigia reais, disco e busca seguinte)");
}
