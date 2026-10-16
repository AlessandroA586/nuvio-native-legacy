// Player externo (#501): Interno / Externo / Perguntar sempre. Ver extplayer.c.
#ifndef NV_EXTPLAYER_H
#define NV_EXTPLAYER_H

enum { EXTPLAYER_INTERNO = 0, EXTPLAYER_EXTERNO = 1, EXTPLAYER_PERGUNTAR = 2 };

// `modo` e o valor do ajuste (0 interno, 1 externo, 2 perguntar); `cabs` os
// cabecalhos do stream ("" = nenhum). Cabecalho nao cabe numa Intent: com ele
// o externo cai no interno e `*avisar` = 1 (quem chama avisa o usuario).
int extplayer_decidir(int modo, const char *cabs, int *avisar);

#ifdef NV_ANDROID
// 1 = tratou (abriu o externo ou abriu a pergunta; `sair` fecha o player);
// 0 = tocar no interno agora. `interno` roda quando a escolha for o interno.
// Nunca registra a URL.
int  extplayer_tentar(const char *url, const char *titulo,
                      void (*interno)(const char *url), void (*sair)(void));
void extplayer_acao(const char *chave, int botao);
void extplayer_passo(unsigned agora);
#endif
#endif
