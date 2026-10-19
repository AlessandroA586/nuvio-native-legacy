// Included once by each native backend after defining FonteContexto,
// IPTV_CTX_INICIALIZAR and IPTV_CTX_EMERGENCIA. Contexts remain allocated so
// workers never retain pointers to an account object freed by the UI.
#include <stdatomic.h>
#include <limits.h>
static pthread_mutex_t fontesTrava = PTHREAD_MUTEX_INITIALIZER;
static FonteContexto *fontesContextos;
static FonteContexto fonteEmergencia = IPTV_CTX_EMERGENCIA;
static atomic_uint contaEditada;
static _Thread_local unsigned fonteExplicita = UINT_MAX;
static _Thread_local unsigned fonteLidaId = UINT_MAX;
static _Thread_local FonteContexto *fonteLida;

static FonteContexto *contexto(void) {
  unsigned id = fonteExplicita == UINT_MAX ? atomic_load(&contaEditada) : fonteExplicita;
  if (fonteLida && fonteLidaId == id) return fonteLida;
  pthread_mutex_lock(&fontesTrava);
  FonteContexto *c = fontesContextos;
  while (c && c->sid != id) c = c->proximo;
  if (!c) {
    c = calloc(1, sizeof *c);
    if (c) {
      c->sid = id; IPTV_CTX_INICIALIZAR(c);
      c->proximo = fontesContextos; fontesContextos = c;
    } else c = &fonteEmergencia; // Never borrow another account on allocation failure.
  }
  pthread_mutex_unlock(&fontesTrava);
  fonteLidaId = id; fonteLida = c;
  return c;
}
static unsigned usarFonte(unsigned id) {
  unsigned anterior = fonteExplicita; fonteExplicita = id; return anterior;
}
#undef IPTV_CTX_INICIALIZAR
#undef IPTV_CTX_EMERGENCIA
