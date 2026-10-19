// Layout Imersiva (experimental): o estado da cena (src/imersiva.c).
#include "ajustes.h"
#include "imersiva.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void anda(Imersiva *s, float ms) { for (float t = 0; t < ms; t += 1000.0f / 60.0f) imv_passo(s, 1.0f / 60.0f, 0); }

int main(void) {
  Imersiva s;
  // Enum: a Imersiva e a quarta e ultima; as outras nao mudam de numero.
  assert(HOME_LAYOUT_MODERNA == 0 && HOME_LAYOUT_PADRAO == 1 && HOME_LAYOUT_DINAMICA == 2);
  assert(HOME_LAYOUT_IMERSIVA == 3 && HOME_LAYOUT_N == 4);
  assert(IMV_TROCA_MS <= 600.0f);

  // Fantasmas: so a fileira de cima e a de baixo, e so se existirem.
  imv_iniciar(&s, 0);
  assert(s.t == 1.0f && s.anterior == -1 && !imv_em_troca(&s));
  assert(imv_fantasma(&s, 5, -1) == -1 && imv_fantasma(&s, 5, +1) == 1);
  imv_iniciar(&s, 4);
  assert(imv_fantasma(&s, 5, -1) == 3 && imv_fantasma(&s, 5, +1) == -1);
  imv_iniciar(&s, 2);
  assert(imv_fantasma(&s, 5, -1) == 1 && imv_fantasma(&s, 5, +1) == 3);
  assert(imv_fantasma(&s, 1, -1) == -1);

  // Troca: relogio fixo, assenta em IMV_TROCA_MS, metade no meio.
  imv_fileira(&s, 3);
  assert(s.anterior == 2 && s.dir == 1 && s.t == 0.0f && imv_em_troca(&s));
  anda(&s, IMV_TROCA_MS * 0.5f);
  assert(fabsf(imv_chega(&s) - 0.5f) < 0.08f && fabsf(imv_sai(&s) - 0.5f) < 0.08f);
  // Escada: o primeiro cartaz vai na frente do quinto.
  assert(imv_escada(&s, 0) > imv_escada(&s, 4));
  anda(&s, IMV_TROCA_MS * 0.5f + 20.0f);
  assert(!imv_em_troca(&s) && imv_chega(&s) == 1.0f && imv_escada(&s, 4) == 1.0f);

  // Interrupcao sem fila: a 30%, a que saia (mais visivel) continua saindo
  // com a mesma opacidade; a nova entra. Nada salta.
  imv_iniciar(&s, 1);
  imv_fileira(&s, 2);
  anda(&s, IMV_TROCA_MS * 0.3f);
  { float sai = imv_sai(&s);
    imv_fileira(&s, 3);
    assert(s.fileira == 3 && s.anterior == 1 && s.dir == 1);
    assert(fabsf(imv_sai(&s) - sai) < 0.01f); }
  // A 70%, a que chegava e a mais visivel: ela vira a que sai, sem salto.
  imv_iniciar(&s, 1);
  imv_fileira(&s, 2);
  anda(&s, IMV_TROCA_MS * 0.7f);
  { float chega = imv_chega(&s);
    imv_fileira(&s, 3);
    assert(s.fileira == 3 && s.anterior == 2);
    assert(fabsf(imv_sai(&s) - chega) < 0.01f); }
  // Voltar no meio: a mesma linha do tempo, ao contrario.
  imv_iniciar(&s, 1);
  imv_fileira(&s, 2);
  anda(&s, IMV_TROCA_MS * 0.4f);
  { float chega = imv_chega(&s);
    imv_fileira(&s, 1);
    assert(s.fileira == 1 && s.anterior == 2 && s.dir == -1);
    assert(fabsf(imv_sai(&s) - chega) < 0.01f); }
  // Muitas teclas: nunca enfileira, a cena assenta UMA troca depois da ultima.
  imv_iniciar(&s, 0);
  for (int i = 1; i <= 6; i++) { imv_fileira(&s, i); anda(&s, 30.0f); }
  anda(&s, IMV_TROCA_MS + 20.0f);
  assert(s.fileira == 6 && !imv_em_troca(&s));
  // Mesma fileira: nada.
  imv_fileira(&s, 6);
  assert(!imv_em_troca(&s));

  // Abrir no lugar / fechar: reversivel e com relogio proprio.
  imv_abrir(&s, 4);
  assert(s.aberto && s.coluna == 4 && s.abre == 0.0f);
  anda(&s, IMV_ABRE_MS * 0.5f);
  assert(s.abre > 0.3f && s.abre < 0.7f);
  imv_fechar(&s);
  { float a = s.abre; anda(&s, 50.0f); assert(s.abre < a); }
  anda(&s, IMV_ABRE_MS);
  assert(!s.aberto && s.abre == 0.0f);
  imv_abrir(&s, 2);
  anda(&s, IMV_ABRE_MS + 20.0f);
  assert(s.abre == 1.0f);
  // Animacoes reduzidas: assenta na hora (o nivel de GPU baixo nao muda o tempo).
  imv_fileira(&s, 5);
  imv_fechar(&s);
  imv_passo(&s, 1.0f / 60.0f, 1);
  assert(!imv_em_troca(&s) && s.abre == 0.0f);
  puts("imersiva: ok");
  return 0;
}
