// Recusa sincrona: video_tocar=0 -> player real em erro -> vigia real -> disco.
#define SDL_MAIN_HANDLED
#include "../src/app.c"
#include <assert.h>
#include "dados.h"
#include "linguas.h"

static int backendFalhou, chamadas, aceita, pronto;
static double pos;
int video_tocar(const char *url) {
  assert(!strcmp(url, "https://example.invalid/recusado.mp4"));
  chamadas++; backendFalhou = !aceita; return aceita;
}
int video_falhou(void) { return backendFalhou; }
int video_pronto(void) { return pronto; }
double video_pos(void) { return pos; }

int main(int argc, char **argv) {
  CatItem c = {0};
  Stream *s = calloc(1, sizeof *s); assert(s);
  assert(!SDL_Init(SDL_INIT_TIMER)); dados_iniciar(""); ling_local_legenda("none");
  strcpy(c.titulo, "Fixture"); strcpy(c.tipo, "movie"); cat_definir(&c, 1);
  strcpy(s->provedor, "Addon"); strcpy(s->url, "https://example.invalid/recusado.mp4");
  stream_definir_lista(s, 1); stream_definir_atual(0);
  player_abrir(0, NULL);
  if (argc > 1) {
    int tardia = !strcmp(argv[1], "tardia");
    VideoSimulacao sim = {0};
    aceita = sim.aceita = 1;
    video_simular(&sim);
    player_definir_fonte(s->url);
    SDL_Delay(1); fonteManualDesde = SDL_GetTicks();
    if (!strcmp(argv[1], "desconhecida")) {
      pronto = 1;
      vigiarAberturaManual();
      assert(fonteManualDesde && "pronto sem progresso desarmou o vigia");
      pos = 1.0;
      vigiarAberturaManual();
      pos = 180.0; backendFalhou = 1;
      vigiarAberturaManual();
      printf("duracao desconhecida: erro aos %.0f s, idade=%ld\n", pos, stream_vod_falhou_ha(0));
      fflush(stdout);
      assert(!fonteManualDesde && stream_vod_falhou_ha(0) < 0 &&
             "erro depois de tocar com duracao desconhecida marcou a fonte");
      fontesmortas_iniciar();
      assert(stream_vod_falhou_ha(0) < 0 && "falha tardia chegou ao disco");
      stream_definir_lista(NULL, 0); free(s);
      puts("fontesmortas: PASS (duracao desconhecida, progresso e erro tardio)");
      return 0;
    }
    if (tardia) {
      fonteManualDesde -= FONTE_MANUAL_PRAZO_MS + 1;
      vigiarAberturaManual();
      assert(!fonteManualDesde && player_fonte_falhou());
    } else stream_vod_falhou(0);
    assert(stream_vod_falhou_ha(0) >= 0);
    pronto = 1; pos = 1.0;
    // Progresso desarma o vigia; so a duracao do titulo pode apagar a marca.
    sim.duracao = 30; video_simular(&sim);
    vigiarAberturaManual();
    assert(stream_vod_falhou_ha(0) >= 0 && "clipe de aviso apagou a marca");
    sim.duracao = 0; video_simular(&sim);
    vigiarAberturaManual();
    assert(stream_vod_falhou_ha(0) >= 0 && "duracao desconhecida apagou a marca");
    sim.duracao = 5400; video_simular(&sim);
    // O loop tira o cartao quando o video volta, antes do vigia manual.
    if (tardia) player_limpar_erro_fonte();
    vigiarAberturaManual();
    assert(!fonteManualDesde && stream_vod_falhou_ha(0) < 0 && "titulo tocando manteve a marca");
    fontesmortas_iniciar();
    assert(stream_vod_falhou_ha(0) < 0 && "limpeza nao chegou ao disco");
    stream_definir_lista(NULL, 0); free(s);
    printf("fontesmortas: PASS (%s, player e vigia reais, disco)\n", argv[1]);
    return 0;
  }
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
