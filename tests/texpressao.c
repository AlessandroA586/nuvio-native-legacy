// PRESSAO DE MEMORIA DO SISTEMA no cache de texturas (Android onTrimMemory).
//
// POR QUE ESTE TESTE EXISTE: um upload de textura chegou a levar 16 s na TV
// Android com o low-memory killer ativo. A resposta e baixar o teto por um
// tempo e despejar arte FRIA — nunca a que esta na tela. Nenhuma tela mostra "o
// aviso foi ouvido"; a regra so se prova aqui, sem contexto GL (tex fica 0).
#include "../src/tex_cache.c"
#include <assert.h>
#include <stdio.h>

static void limpar(void) {
  memset(itens, 0, sizeof itens);
  nMax = 8; quadroAtual = 100; bytesUsados = 0;
  tex_despejos = 0; tex_despejos_quentes = 0;
  pressaoPct = 100; pressaoAte = 0; pressaoPlay = 100;
  orcMemTotal = 0;
}
// 1024x10: bytesTextura sem piramide, conta redonda.
static void pronto(int i, unsigned long uso, unsigned long quadro) {
  itens[i].estado = PRONTO; itens[i].uso = uso; itens[i].ultimoQuadro = quadro;
  itens[i].w = 1024; itens[i].h = 10;
  snprintf(itens[i].caminho, sizeof itens[i].caminho, "a%d", i);
  bytesUsados += bytesTextura(&itens[i]);
}

#include "../src/memlog.h"
static long livres[2];
static int nLivre, aliviou;
static long livreFalso(void) { return livres[nLivre++ & 1]; }
static void aliviarFalso(void) { aliviou++; }

int main(void) {
  long um = bytesTexturaMip(1024, 10, 0);
  int i;
  mtx = SDL_CreateMutex();   // tex_novo_quadro sai sem ele (cache nao iniciado)

  // 1. A tabela de niveis do Android.
  assert(tex_pressao_pct(0) == 100 && tex_pressao_pct(4) == 100);
  assert(tex_pressao_pct(5) == 80 && tex_pressao_pct(10) == 60 && tex_pressao_pct(15) == 40);
  assert(tex_pressao_pct(20) == 50 && tex_pressao_pct(40) == 50);
  assert(tex_pressao_pct(60) == 25 && tex_pressao_pct(80) == 25);
  puts("ok  tabela de niveis");

  // 2. Sem aviso, nada muda: cache cheio no teto continua no teto.
  limpar();
  orcamento = um * 8;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);   // todos frios
  podar();
  assert(tex_despejos == 0 && bytesUsados == um * 8);
  puts("ok  sem aviso nao despeja");

  // 3. Aviso CRITICO (15 = 40%): o proximo quadro despeja frios ate caber.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_pressao_memoria(15);
  assert(pressaoPct == 40);
  tex_novo_quadro();
  assert(bytesUsados <= um * 4);              // 40% de 10 = 4
  assert(tex_despejos == 4);
  assert(itens[0].estado == VAZIO && itens[7].estado == PRONTO);   // LRU: sai o de menor uso
  puts("ok  aviso critico despeja frios no quadro seguinte");

  // 4. NUNCA despeja arte quente (desenhada neste quadro), nem sob aviso.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 6; i++) pronto(i, (unsigned long)i + 1, 100);  // todos quentes
  tex_pressao_memoria(80);
  tex_novo_quadro();
  assert(tex_despejos == 0 && bytesUsados == um * 6);
  puts("ok  arte quente fica, mesmo sob aviso");

  // 5. Aviso mais fraco depois de um forte nao afrouxa.
  limpar();
  orcamento = um * 10;
  tex_pressao_memoria(15);
  tex_pressao_memoria(5);
  assert(pressaoPct == 40);
  puts("ok  aviso fraco nao afrouxa o forte");

  // 6. O teto volta sozinho passado o prazo.
  limpar();
  orcamento = um * 10;
  tex_pressao_memoria(15);
  pressaoAte = SDL_GetTicks() - 1;           // prazo vencido
  assert(!pressaoAtiva() && pressaoPct == 100);
  assert(limiteBytes() == orcamento);
  puts("ok  teto volta ao normal sozinho");

  // 7. Nivel sem efeito (< 5) nao liga nada.
  limpar();
  tex_pressao_memoria(0);
  assert(pressaoPct == 100 && !pressaoAtiva());
  puts("ok  nivel baixo nao liga a pressao");

  // 8. REPRODUCAO (tex_pressao_reproducao): so Android liga (aqui o define
  //    nao existe, entao o teto e 100%% e nada muda) — a regra do C que vale
  //    em qualquer plataforma e que fechar o video devolve o teto inteiro.
  limpar();
  orcamento = um * 10;
  orcMemTotal = 2400;                       // a TCL do incidente
#ifdef NV_ANDROID
  assert(tex_pressao_play_pct() == 40);     // < 3 GB
  assert((orcMemTotal = 4096, tex_pressao_play_pct()) == 75);
  assert((orcMemTotal = 2048, tex_pressao_play_pct()) == 40);
  orcMemTotal = 2400;
  tex_pressao_reproducao(1);
  assert(pressaoPlay == 40);
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_novo_quadro();
  assert(bytesUsados <= um * 4);            // 40%% de 10
  assert(tex_despejos == 4);
  tex_pressao_reproducao(0);                // fechou o video: teto volta
  assert(pressaoPlay == 100 && limiteBytes() == orcamento);
#else
  assert(tex_pressao_play_pct() == 20);     // webOS/.tpk: piso, ligado so sem memoria
  tex_pressao_reproducao(1);
  assert(pressaoPlay == 20);
  tex_pressao_reproducao(0);
  assert(pressaoPlay == 100 && limiteBytes() == orcamento);
#endif
  puts("ok  reproducao aperta o teto e devolve ao fechar");

#ifndef NV_ANDROID
  // 9. #415: MemAvailable alto (a C9) nao muda nada, nem com o player aberto.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_memoria_livre(900, 1);
  tex_memoria_livre(-1, 1);                 // sem /proc: nada
  tex_novo_quadro();
  assert(pressaoPlay == 100 && pressaoPct == 100 && tex_despejos == 0);
  puts("ok  memoria sobrando nao mexe no teto");

  // 10. Player aberto com a G4 a 250 MB livres: piso de reproducao (20%%) no
  //     quadro seguinte; fica no piso enquanto o video durar, volta ao fechar.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_memoria_livre(250, 1);
  assert(pressaoPlay == 20 && pressaoPct == 50);
  tex_novo_quadro();
  assert(bytesUsados <= um * 2 && tex_despejos == 6);
  tex_memoria_livre(600, 1);
  assert(pressaoPlay == 20);
  tex_memoria_livre(600, 0);
  assert(pressaoPlay == 100);
  puts("ok  player com pouca memoria desce o cache ao piso e solta ao fechar");

  // 11. O aviso (< 150 MB) sem player: teto a 25%% e despejo no quadro seguinte.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_memoria_livre(92, 0);
  assert(pressaoPct == 25 && pressaoPlay == 100);
  tex_novo_quadro();
  assert(bytesUsados <= um * 2 + um / 2 && tex_despejos == 6);
  puts("ok  aviso de memoria despeja arte fria");

  // 12. tex_aliviar despeja na hora, sem esperar o quadro.
  limpar();
  orcamento = um * 10;
  for (i = 0; i < 8; i++) pronto(i, (unsigned long)i + 1, 50);
  tex_aliviar();
  assert(pressaoPct == 25 && bytesUsados <= um * 2 + um / 2);
  puts("ok  aliviar despeja agora");
#endif

  // 13. Reabrir o pipeline depois de um erro: abaixo do piso alivia e le de
  //     novo; ainda abaixo, nao reabre.
  livres[0] = 92; livres[1] = 92; nLivre = 0; aliviou = 0;
  assert(!memlog_pode_reabrir(livreFalso, aliviarFalso) && aliviou == 1);
  livres[0] = 92; livres[1] = 200; nLivre = 0; aliviou = 0;
  assert(memlog_pode_reabrir(livreFalso, aliviarFalso) && aliviou == 1);
  livres[0] = 400; nLivre = 0; aliviou = 0;
  assert(memlog_pode_reabrir(livreFalso, aliviarFalso) && aliviou == 0);
  livres[0] = -1; nLivre = 0; aliviou = 0;
  assert(memlog_pode_reabrir(livreFalso, aliviarFalso) && aliviou == 0);
  puts("ok  reabrir apos erro so com memoria acima do piso");
  puts("PASS: texpressao");

  return 0;
}
