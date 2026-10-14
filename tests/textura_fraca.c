// TEXTURA FRACA: a troca do recorte chapado pelo material CC0 (cb633be2)
// levava a SUPERFICIE do material mas deixava a TINTA do rotulo na do recorte
// da arte, e media o veu pelo par de percentis do material-000 para todos os
// arquivos (revisao, 14/10). O rotulo de TODO botao vem de
// ajustes_acento_tinta -> nv_textura_viva.tintaBranca; o material aqui nao
// existe no disco (caminho inexistente, so para o swap rodar): o que interessa
// e o que a troca escreve de volta no struct, ANTES de qualquer tex_*.
//
//   bash tests/textura_fraca.sh
#include "../src/ajustes.c"
#include <assert.h>
#include <string.h>

int main(void) {
  CorvivaTextura *T = &nv_textura_viva;
  ajustes_texturas_dir("/tmp/nao-existe-nvio-textura");
  // Recorte FRACO de arte escura: a arte diz tinta BRANCA (tintaBranca=1),
  // mas o material de Y~0,42 diz ESCURA — sem a escrita de volta o rotulo
  // vinha BRANCO sobre o veu claro do material (~2,2:1; a regra e 4,5:1).
  valor[AJ_TEMA] = AJ_TEMA_TEXTURA;
  memset(T, 0, sizeof *T);
  T->ok = 1; T->fraca = 1; T->forca = 1.0f; T->tintaBranca = 1;
  strcpy(T->url, "x");
  ajustes_textura_quadro();
  assert(!T->tintaBranca);
  assert(ajustes_acento_tinta(NULL, NULL, NULL) == AJ_TINTA_ESCURA);
  // SUTIL (forca 0,35) com base em Y~0,05 (bY 0,25): o veu tem que sair do par
  // do ARQUIVO que entra — o NEUTRO (0,1845/0,7913) pede veu 0,05; o par do
  // material-000 (0,1861/0,6935) deixaria veu 0, contraste ~4,3:1 no neutro
  // real de p90 0,79.
  valor[AJ_TEMA] = AJ_TEMA_TEXTURA_SUTIL;
  memset(T, 0, sizeof *T);
  T->ok = 1; T->fraca = 1; T->forca = 0.35f; T->tintaBranca = 1;
  T->base[0] = T->base[1] = T->base[2] = 0.25f;
  strcpy(T->url, "x");
  ajustes_textura_quadro();
  assert(T->tintaBranca);
  assert(T->veu >= 0.05f);
  // O mesmo sutil com ACENTO VERMELHO: faixa 030, o par do material-030
  // (0,1889/0,6868) nao pede veu nenhum — o veu tambem prova que o par
  // seguiu o arquivo da faixa do acento, e nao um fixo.
  nv_acento_viva[0] = 1.0f; nv_acento_viva[1] = nv_acento_viva[2] = 0.0f;
  memset(T, 0, sizeof *T);
  T->ok = 1; T->fraca = 1; T->forca = 0.35f; T->tintaBranca = 1;
  T->base[0] = T->base[1] = T->base[2] = 0.25f;
  strcpy(T->url, "x");
  ajustes_textura_quadro();
  assert(T->tintaBranca);
  assert(T->veu < 0.05f);
  puts("ok  textura fraca: tinta e veu do material CC0, nao do recorte da arte");
  return 0;
}
