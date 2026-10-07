// TELA NOVA DO TESTE DE VELOCIDADE (src/vazao.c): latencia/jitter, as tres
// faixas da tabela de qualidade e o veredito, sem rede. Ver vazao_tabela.sh.
#include "vazao.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  int med, jit, bom, prox;
  // 38, 44, falha, 36, 40: mediana 39, jitter (6 + 8 + 4)/3 = 6.
  { int ms[5] = { 38, 44, -1, 36, 40 };
    assert(vazao_latencia(ms, 5, &med, &jit) == 4);
    assert(med == 39 && jit == 6); }
  { int ms[3] = { -1, -1, -1 };
    assert(vazao_latencia(ms, 3, &med, &jit) == 0 && med == 0 && jit == 0); }
  // O mockup: mediana 53, pior trecho 41,2 -> otimo 30,9, maximo 47,7.
  { VazaoResumo r;
    memset(&r, 0, sizeof r);
    r.n = 24; r.medianaKbps = 53000; r.p20Kbps = 41200;
    r.otimoKbps = 30900; r.maximoKbps = 47700;
    assert(vazao_banda(&r, vazao_qualidade(0)->tipicoKbps) == VBANDA_TOCA);
    assert(vazao_banda(&r, vazao_qualidade(2)->tipicoKbps) == VBANDA_TOCA);          // 4K HDR 28
    assert(vazao_banda(&r, vazao_qualidade(3)->tipicoKbps) == VBANDA_PODE_TRAVAR);   // 4K remux 45
    assert(vazao_banda(&r, vazao_qualidade(4)->tipicoKbps) == VBANDA_NAO_TOCA);      // DV remux 80
    vazao_veredito(&r, &bom, &prox);
    assert(bom == 2 && prox == 3);
    // Tudo toca bem: sem proxima.
    r.otimoKbps = 90000; r.maximoKbps = 99000;
    vazao_veredito(&r, &bom, &prox);
    assert(bom == 4 && prox == -1);
    // Nem 1080p.
    r.otimoKbps = 5000; r.maximoKbps = 7000;
    vazao_veredito(&r, &bom, &prox);
    assert(bom == -1 && prox == 0);
    assert(vazao_banda(&r, 8000) == VBANDA_NAO_TOCA); }
  assert(!strcmp(vazao_qualidade(4)->nome, "4K Dolby Vision remux") && !vazao_qualidade(5));
  puts("PASS: tabela do teste de velocidade");
  return 0;
}
