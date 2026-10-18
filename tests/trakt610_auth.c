// #610: exercise the real auth state machine with deterministic HTTP responses.
#include "stub_perfisessao.h"
#define main perfil_main
#define rede_postar_st_retry perfil_postar_retry
#include "traktauth_perfil.c"
#undef rede_postar_st_retry
#undef main
#include "../src/traktauth.c"
int sessao_identidade_local_copiar(char *u, size_t nu, char *j, size_t nj) {
  (void)nu; (void)nj; u[0] = j[0] = 0; return 0;
}
const char *sessao_usuario(void) { return ""; }

static int respostas429 = 1, retry = 90, pedidos;
char *rede_postar_st_retry(const char *u, int s, const char *const *c,
                           const char *b, int *st, int *ra) {
  pedidos++;
  if (respostas429) { respostas429--; *st = 429; *ra = retry; return strdup(""); }
  *ra = 0;
  return rede_postar_st(u, s, c, b, st);
}
static void terminar(void) {
  for (int i = 0; i < 3000 && !fioPronto; i++) usleep(1000);
  assert(fioPronto);
}
int main(void) {
  traktauth_comecar(); terminar();
  assert(traktauth_estado() == TRA_PEDINDO && !traktauth_erro()[0]);
  traktauth_passo(1000);
  traktauth_passo(90999); assert(pedidos == 1);
  traktauth_passo(91000); terminar();
  assert(traktauth_estado() == TRA_AGUARDANDO);
  respostas429 = 1; retry = 120;
  traktauth_passo(92000); terminar();
  assert(traktauth_estado() == TRA_AGUARDANDO);
  traktauth_passo(93000);
  int antes = pedidos;
  traktauth_passo(212999); assert(pedidos == antes);
  traktauth_passo(213000); terminar();
  traktauth_passo(214000); assert(traktauth_estado() == TRA_LIGADO);
  // Repeated 429 without a header doubles the delay; Back does not bypass it.
  traktauth_esquecer();
  respostas429 = 3; retry = 0;
  traktauth_comecar(); terminar();
  traktauth_passo(300000);
  traktauth_cancelar(); traktauth_comecar();
  antes = pedidos;
  traktauth_passo(309999); assert(pedidos == antes);
  traktauth_passo(310000); terminar(); traktauth_passo(311000);
  antes = pedidos;
  traktauth_passo(330999); assert(pedidos == antes);
  traktauth_passo(331000); terminar(); traktauth_passo(332000);
  antes = pedidos;
  traktauth_passo(371999); assert(pedidos == antes);
  traktauth_passo(372000); terminar();
  // Refresh 429 preserves the session and observes headers longer than 60 s.
  traktauth_esquecer();
  snprintf(token, sizeof token, "OLD"); snprintf(refresh, sizeof refresh, "R1");
  usuarioCopiar(usuarioEstado); estado = TRA_LIGADO; renovacaoPend = 1;
  respostas429 = 1; retry = 180;
  traktauth_passo(400000); terminar(); traktauth_passo(401000);
  antes = pedidos;
  traktauth_passo(580999); assert(pedidos == antes);
  traktauth_passo(581000); terminar(); traktauth_passo(582000);
  assert(traktauth_estado() == TRA_LIGADO && !strcmp(token, "RENOVADO-P1"));
  traktauth_desconectar();
  assert(traktauth_perfil() == 1 && !trakt_ativo() && traktauth_desconectado());
  assert(!traktauth_carregar_perfil(1) && traktauth_desconectado());
  // A response from the disconnected generation cannot recreate its token.
  gerFio = geracao - 1; perfilFio = 1; usuarioCopiar(usuarioFio);
  snprintf(refreshFio, sizeof refreshFio, "R1");
  fioRenovar(NULL);
  assert(contem("trakt-p1.txt", "!") && !token[0]);
  traktauth_comecar(); terminar();
  traktauth_passo(700000); terminar(); traktauth_passo(701000);
  assert(traktauth_estado() == TRA_LIGADO && !traktauth_desconectado());
  puts("#610 auth: PASS (Retry-After, backoff, profile disconnect, stale response, reconnect)");
  return 0;
}
