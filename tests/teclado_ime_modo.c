// Ajuste "Teclado" (Android): 0 automatico (o de sempre), 1 so a grade do app,
// 2 o teclado do sistema abre direto em todo campo. st_teste_ligar(1) faz o
// teste fazer o papel do Android.
#include "ajustes.h"
#include "dados.h"
#include "sistexto.h"
#include "teclado.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char dir[] = "/tmp/nuvio-teclado-ime-XXXXXX", caminho[700];
static void modo(int v) {
  FILE *f = fopen(caminho, "w"); assert(f);
  fprintf(f, "tecladoAndroidLocal %d\n", v); fclose(f);
  ajustes_dir(dir);
  assert(ajustes_teclado_android() == v);
}
static int imeAberto(void) { return st_estado() == ST_DIGITANDO && st_dono() == ST_TECLADO; }
static void abrir(int tipo) {
  st_fechar(ST_TECLADO);
  teclado_abrir("Titulo", "dica", 40);
  if (tipo >= 0) teclado_tipo(tipo);
  // O modo "Do Android" adia a abertura um quadro (teclado_tipo pode trocar
  // o tipo logo depois do abrir).
  teclado_atualizar(0, 0);
}

int main(void) {
  assert(mkdtemp(dir));
  assert(setenv("NUVIO_DADOS", dir, 1) == 0);
  dados_iniciar(dir);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  st_teste_ligar(1);

  // Automatico: campo de texto abre a grade; e-mail e senha abrem o sistema.
  modo(0);
  abrir(TECLADO_TIPO_TEXTO); assert(!imeAberto() && teclado_foco_campo() == 0);
  abrir(TECLADO_TIPO_EMAIL); assert(imeAberto());
  abrir(TECLADO_TIPO_SENHA); assert(imeAberto());

  // Do app: o sistema nunca abre sozinho, nem em e-mail/senha; a grade tem foco.
  modo(1);
  abrir(TECLADO_TIPO_TEXTO); assert(!imeAberto() && teclado_foco_campo() == 0);
  abrir(TECLADO_TIPO_EMAIL); assert(!imeAberto() && teclado_foco_campo() == 0);
  abrir(TECLADO_TIPO_SENHA); assert(!imeAberto() && teclado_foco_campo() == 0);

  // Do Android: abre direto em todo campo, com o botao Teclado da TV em foco
  // (o caminho de volta para a grade, que Voltar tambem mostra).
  modo(2);
  abrir(-1); assert(imeAberto() && teclado_foco_campo() != 0);
  abrir(TECLADO_TIPO_TEXTO); assert(imeAberto());
  abrir(TECLADO_TIPO_EMAIL); assert(imeAberto());

  // Fora do Android o ajuste nao muda nada. st_teste_ligar(0) desliga o
  // "abre sozinho", e teclado_teste_modos(1, ...) faz o papel do teclado de
  // sistema DISPONIVEL (TV Samsung .tpk, web): e-mail/senha tem que continuar
  // abrindo a GRADE em foco, nao o IME por cima da modal.
  st_teste_ligar(0);
  teclado_teste_modos(1, -1, -1);
  abrir(TECLADO_TIPO_TEXTO); assert(!imeAberto() && teclado_foco_campo() == 0);
  abrir(TECLADO_TIPO_EMAIL); assert(!imeAberto() && teclado_foco_campo() == 0);
  abrir(TECLADO_TIPO_SENHA); assert(!imeAberto() && teclado_foco_campo() == 0);
  teclado_teste_modos(-1, -1, -1);
  puts("teclado_ime_modo: ok");
  return 0;
}
