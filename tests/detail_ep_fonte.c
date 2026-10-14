// ISSUE #513: O TOQUE NO CARD DE EPISODIO respeita "Escolher a fonte ao
// reproduzir" (escolherFonteManual = o auto-play desligado), sem janela e sem
// rede.
//
// Relato (Samsung, 2.0.5): serie em "Continuar assistindo", QUALQUER episodio
// escolhido na pagina do titulo tocava direto, sem a folha de fontes — a fonte
// lembrada (fontepref) e a da ultima sessao (fontevolta) entravam antes de
// perguntar. Serie fora da retomada mostrava a folha como esperado.
//
// A prova: serie COM progresso (a condicao do Continuar assistindo) e ajuste
// ligado; OK curto num card de episodio tem de pedir a FOLHA (pedFontes) e
// nao a reproducao (pedReproduzir). Com o ajuste desligado o toque continua
// tocando, como sempre. E o "Retomar" do Continuar assistindo — o pedido
// vindo de fora (detail_pedir_reproduzir) — nao muda: ele e quem pode reusar
// a fonte guardada.
#include "nvtmp.h"
#include "dados.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/detail.c"

static char dir[NVT_MAX], caminho[700];
static void gravar(const char *txt) {
  FILE *f = fopen(caminho, "w");
  assert(f);
  fputs(txt, f);
  fclose(f);
  ajustes_dir(dir);
}

// OK curto = KEYDOWN seguido de KEYUP sem esperar (dur < NV_HOLD_MS), como um
// toque no controle. Em serie, na fileira de episodios.
static void toqueOk(void) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RETURN;
  detail_evento(&e);
  e.type = SDL_KEYUP;
  detail_evento(&e);
}

int main(void) {
  CatItem c;
  CatEp eps[6];
  int i;

  nvt_set(dir, sizeof dir, "/nuvio-detail-epfonte-XXXXXX");
  assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  ajustes_iniciar();

  // Uma serie COM progresso: e a condicao para entrar em "Continuar
  // assistindo" — progresso no T1E3 e 6 episodios na T1.
  memset(&c, 0, sizeof c);
  snprintf(c.tipo, sizeof c.tipo, "series");
  snprintf(c.titulo, sizeof c.titulo, "Série de Ensaio");
  snprintf(c.imdb, sizeof c.imdb, "tt0000513");
  c.temporadas[0] = 1; c.nTemporadas = 1;
  c.progresso = 40; c.temporada = 1; c.episodio = 3;
  cat_definir_tudo(&c, 1, NULL, 0); idx = 0;
  memset(eps, 0, sizeof eps);
  for (i = 0; i < 6; i++) {
    eps[i].temporada = 1;
    eps[i].episodio = i + 1;
    snprintf(eps[i].nome, sizeof eps[i].nome, "Episódio %d", i + 1);
  }
  cat_definir_episodios(0, eps, 6);

  aberto = 1; saindo = 0; nivel = 1;
  foco.fileira = SEC_EPISODIOS; foco.coluna = 1;   // um OUTRO episodio (E2)
  for (i = 0; i < 10; i++) detail_atualizar(1.0f / 60.0f, 1000 + i * 16);

  // AUTOMATICO (padrao): o toque curto toca — o caso do #57, mantido.
  gravar("escolherFonteManual 1\n");   // V_LIGA: 1 = Desligado
  assert(!ajustes_fonte_manual());
  toqueOk();
  assert(detail_pediu_reproduzir() == 1 && detail_pediu_fontes() == 0);
  puts("ok com o automatico: o toque no episodio toca direto");

  // AUTO-PLAY DESLIGADO ("Escolher a fonte ao reproduzir" ligado): o toque num
  // OUTRO episodio abre a folha de fontes, nao a reproducao.
  gravar("escolherFonteManual 0\n");   // V_LIGA: 0 = Ligado
  assert(ajustes_fonte_manual());
  toqueOk();
  assert(detail_pediu_fontes() == 1 && detail_pediu_reproduzir() == 0);
  puts("ok com escolha manual: outro episodio abre a folha de fontes");

  // O MESMO episodio do progresso (T1E3, o que o card de Continuar assistindo
  // anunciava): escolhido a dedo na fileira, tambem abre a folha.
  foco.coluna = 2;
  for (i = 0; i < 10; i++) detail_atualizar(1.0f / 60.0f, 2000 + i * 16);
  toqueOk();
  assert(detail_pediu_fontes() == 1 && detail_pediu_reproduzir() == 0);
  puts("ok com escolha manual: o episodio do progresso tambem abre a folha");

  // O "RETOMAR" DE FORA (o OK no card de Continuar assistindo, ou o primario
  // quando ele e "Retomar T1E3") continua sendo um PEDIDO DE REPRODUCAO: e o
  // gesto desenhado para reusar a fonte guardada, e nao muda.
  detail_pedir_reproduzir();
  assert(detail_pediu_reproduzir() == 1 && detail_pediu_fontes() == 0);
  puts("ok o Retomar do Continuar assistindo segue pedindo reproducao");

  puts("detail_ep_fonte: o toque no episodio obedece \"Escolher a fonte ao reproduzir\"");
  return 0;
}
