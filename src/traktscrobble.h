// TRAKT SCROBBLE (#179): a decisao de QUAL chamada mandar e o corpo dela, sem
// rede nem estado global — para o teste poder cobrir sem SDL.
//
// O Trakt so mostra "Now Watching" com /scrobble/start; /scrobble/pause guarda
// o ponto e /scrobble/stop com >= 90% (limiar deste cliente) fecha o historico.
// Ate a 1.5.3 o app so mandava pause/stop, uma vez, ao SAIR do player.
#ifndef NV_TRAKTSCROBBLE_H
#define NV_TRAKTSCROBBLE_H
#include <stddef.h>

enum { SCR_NADA = 0, SCR_START, SCR_PAUSE, SCR_STOP };
enum { SCR_EV_TOCANDO = 1, SCR_EV_PAUSOU, SCR_EV_SAIU };

typedef struct { char id[64]; int ativo; } ScrobbleEstado;

// Devolve a chamada a fazer (SCR_*) e avanca o estado.
//   TOCANDO: start, salvo se ja ha um start em pe para o MESMO id (sem spam).
//   PAUSOU : pause, so se ha start em pe para o id (pausar de novo e ruido).
//   SAIU   : sempre fala: stop a partir de 90%, senao pause (guarda o ponto).
int scrobble_decidir(ScrobbleEstado *s, int evento, const char *id, double pct);

// PROVA DE REPRODUCAO do stop/visto automatico (#205). So segura o "seek ate o
// fim": metade do que faltava quando a soma comecou (`deSeg`, o ponto da
// retomada; a soma vive em progresso.c, entre sessoes) tem de ter sido tocada
// de verdade, e nunca menos de 2 min
// (duracao falsa de trailer, 129 s). Abertura, recap e creditos pulados cabem
// na outra metade — exigir 90% da duracao desfazia o #100. `assistidoSeg` e
// MIDIA tocada, nao relogio: a 1,5x o titulo inteiro conta inteiro.
// ponytail: fracao fixa; quem toca metade e busca o fim marca. Creditar cada
// pulo por marcador so se isso aparecer em relato.
// Inline: o selo do catalogo (progresso.c) le a mesma regra sem linkar o Trakt.
static inline int scrobble_assistiu(double assistidoSeg, double deSeg, double durSeg) {
  double metade = (durSeg - deSeg) * 0.5;
  return assistidoSeg >= (metade > 120.0 ? metade : 120.0);
}

// JSON do corpo. `id` e "tt123" (filme) ou "tt123:T:E" (episodio: id do SHOW
// mais temporada/numero, que o Trakt resolve sozinho).
void scrobble_corpo(char *dst, size_t n, const char *id, double pct);
const char *scrobble_nome(int acao);
#endif
