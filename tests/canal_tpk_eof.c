// #302: politica real do watchdog, compilada para TPK e para LG/Android.
#include "../src/livestall.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  NvLiveStall s;
  unsigned t = 1000;
  nv_ls_zerar(&s);
  // Mais EOFs que o limite; inclusive cortes antes dos 20 s de estabilidade.
  const double cortes[] = {24.1, 31.5, 31.7, 35.3, 2.0, 5.0};
  for (unsigned i = 0; i < sizeof cortes / sizeof cortes[0]; i++) {
    assert(nv_ls_passo(&s, t, 1.0, 1, 0, 0, 0) == NV_LS_NADA);
    t += (unsigned)(cortes[i] * 1000);
    // EOF pode chegar junto com a ultima atualizacao da posicao.
    int r = nv_ls_passo(&s, t, cortes[i], 1, 0, 1, 0);
#ifdef NV_TPK
    assert(r == NV_LS_REABRIR);
    assert(s.tentativa == 0 && !s.pendente);
#else
    assert(r == NV_LS_NADA); // comportamento anterior: primeiro observa progresso
    assert(nv_ls_passo(&s, t + 1, cortes[i], 1, 0, 1, 0) == NV_LS_AGENDOU);
    assert(s.tentativa == 1 && s.quando == t + 1001);
    nv_ls_zerar(&s);
#endif
    nv_ls_reaberto(&s, t);
    assert(nv_ls_passo(&s, t + 1, 0, 0, 0, 0, 0) == NV_LS_NADA);
    t += 100;
  }
  nv_ls_zerar(&s);
  nv_ls_passo(&s, t, 1, 1, 0, 0, 0);
  assert(nv_ls_passo(&s, t + 1, 1, 1, 1, 1, 0) == NV_LS_NADA);
#ifdef NV_TPK
  // Primeiro quadro observado ja tem EOF; ou o backend zera a posicao no EOF.
  nv_ls_zerar(&s);
  assert(nv_ls_passo(&s, t, 24.1, 1, 0, 1, 0) == NV_LS_REABRIR);
  nv_ls_zerar(&s);
  nv_ls_passo(&s, t, 1, 1, 0, 0, 0);
  assert(nv_ls_passo(&s, t + 1, 0, 1, 0, 1, 0) == NV_LS_REABRIR);
  // EOF nao consome nem apaga tentativas anteriores de trava.
  s.tentativa = 2;
  assert(nv_ls_passo(&s, t + 2, 1, 1, 0, 1, 0) == NV_LS_REABRIR);
  assert(s.tentativa == 2);
#endif
  // Fonte que nunca entregou video continua limitada; EOF nao cria loop livre.
  nv_ls_zerar(&s);
  assert(nv_ls_passo(&s, t, 0, 0, 0, 1, 0) == NV_LS_NADA);
  for (int i = 1; i <= NV_LS_MAX; i++) {
    assert(nv_ls_passo(&s, t, 0, 1, 0, 1, 0) == NV_LS_AGENDOU);
    assert(s.tentativa == i);
    assert(nv_ls_passo(&s, t, 0, 1, 0, 1, 0) == NV_LS_ESPERA);
    t = s.quando;
    assert(nv_ls_passo(&s, t, 0, 1, 0, 1, 0) == NV_LS_REABRIR);
    nv_ls_reaberto(&s, t);
  }
  assert(nv_ls_passo(&s, t, 0, 1, 0, 1, 0) == NV_LS_DESISTIR);
  // Trava/buffering continuam com backoff e limite, inclusive no TPK.
  nv_ls_zerar(&s);
  nv_ls_passo(&s, t, 1, 1, 0, 0, 0);
  assert(nv_ls_passo(&s, t + 8000, 1, 1, 0, 0, 8000) == NV_LS_AGENDOU);
  assert(s.tentativa == 1 && s.quando == t + 9000);
  puts("ok: EOF de live, pausa, abertura e backoff de trava");
}
