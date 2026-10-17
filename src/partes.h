// OS ARQUIVOS DE UMA FONTE, E O FILME EM PARTES (dono, 16/10/2026).
//
// O addon responde UM link por titulo. Um filme que saiu cortado em episodios
// (Kaguya-sama "The First Kiss That Never Ends": 96 min ou 4 x 25 min) vem
// num torrent so, e a fonte mostrava apenas a parte 1. Segurar OK numa fonte
// (folha de Fontes) abre "Arquivos desta fonte": os videos do item na conta
// TorBox da pessoa, na ordem do torrent. Escolher um toca aquele arquivo.
//
// ACHAR O ITEM DA FONTE NO TORBOX, nesta ordem (o log diz qual serviu):
//   1. o hash que o addon mandou (infoHash / clientResolve.infoHash);
//   2. o nome do arquivo (behaviorHints.filename) num arquivo de algum item;
//   3. um hash de 40 hex dentro do link do addon (MediaFusion poe o hash no
//      caminho) — so comparado com os hashes da conta, nunca enviado.
// Nao achou e o addon mandou o hash: o torrent entra na conta SO se ja estiver
// em cache no TorBox (o mesmo que o addon faz ao tocar).
//
// O FILME EM PARTES: tocando um arquivo da lista que tem duas partes ou mais
// (extras como NCOP/NCED/sample ficam de fora da sequencia), a SESSAO DE
// PARTES fica ligada para aquele alvo:
//   - no fim de uma parte a seguinte toca sozinha (o link e pedido 60 s antes);
//   - o progresso que vai ao Continuar assistindo, ao Trakt e a conta e o do
//     FILME INTEIRO, estimado pelo tamanho dos arquivos (partes_virtual);
//   - a parte e o ponto dentro dela ficam guardados no disco por conta e
//     perfil: o Retomar volta na mesma parte, no mesmo ponto.
// Escolher outra fonte para o titulo desliga a sessao e esquece o ponto.
//
// Nunca vai para log: link do addon, link do CDN, chave.
#ifndef NV_PARTES_H
#define NV_PARTES_H
#include <stdint.h>
#include "streams.h"

#define PARTES_MAX 64
#define PARTES_NOME 192
#define PARTES_ARQUIVO "partes.txt"
#define PARTES_GUARDADAS 50
// Um ponto guardado vale 30 dias. O TorBox apaga itens parados depois de um
// tempo; vencido ou sumido, o Retomar cai na escolha normal.
#define PARTES_VALIDADE_S (30L * 24 * 3600)

enum {
  PARTES_VAZIO = 0, PARTES_BUSCANDO, PARTES_PRONTO,
  PARTES_SEM_CHAVE, PARTES_SEM_PLANO, PARTES_NAO_ACHOU, PARTES_ERRO
};

typedef struct {
  int id;                  // file_id do TorBox
  char nome[PARTES_NOME];  // so o nome, sem as pastas
  uint64_t bytes;
  int extra;               // NCOP/NCED/sample/trailer ou bem menor que o maior
} ParteArq;

typedef struct {
  int estado;              // PARTES_*
  int usenet;              // 1 = item de usenet, 0 = torrent
  int item;                // id do item no TorBox
  int n;
  ParteArq arq[PARTES_MAX];   // so videos, na ordem natural do caminho
  int atual;               // o arquivo que a fonte toca (-1 = nao se sabe)
  char como[24];           // "hash", "filename", "link hash", "added"
} PartesLista;

// ---- puro (testes) --------------------------------------------------------
// Hashes de 40 hex soltos no texto, em minusculas. Devolve quantos.
int  partes_hashes_texto(const char *texto, char saida[][41], int max);
// Le um item do mylist (o objeto que comeca em `item`) para `l`: id, videos
// ordenados, extras marcados. 1 se tem ao menos um video.
int  partes_ler_item(const char *item, const char *fim, int usenet, PartesLista *l);
// Procura no corpo de um mylist (data = lista de itens, ou um item so) o item
// da fonte. `hashes` (n >= 0) sao comparados com o hash de cada item;
// `arquivo` (pode ser vazio) com o nome de cada arquivo; `bytes` (0 = nao se
// sabe) desempata. 1 achou, com `l` preenchida e `atual` no arquivo da fonte.
int  partes_achar(const char *corpo, int usenet, char hashes[][41], int nHashes,
                  int hashesConfiaveis, const char *arquivo, uint64_t bytes, PartesLista *l);
// Quantas partes de verdade (nao extras) a lista tem.
int  partes_principais(const PartesLista *l);
// A parte de verdade seguinte a `k`, ou -1.
int  partes_seguinte(const PartesLista *l, int k);
// Numero (1..) de `k` entre as partes de verdade; 0 se `k` e extra.
int  partes_numero(const PartesLista *l, int k);
// A posicao no FILME INTEIRO, pelo tamanho dos arquivos: tudo o que vem antes
// de `k` mais a fracao tocada de `k`. `catDur` (0 = nao se sabe) e a duracao
// do catalogo; sem ela, a estimativa e a duracao desta parte escalada pelo
// tamanho. 1 = *vpos/*vdur validos.
int  partes_conta_virtual(const PartesLista *l, int k, double pos, double dur, double catDur,
                          double *vpos, double *vdur);

// ---- busca da lista (fio proprio) -----------------------------------------
// Pede a lista de arquivos da fonte. A mesma fonte, ja achada nesta sessao do
// app, responde na hora pela memoria.
void partes_pedir(const Stream *fonte);
// Copia o estado da ultima busca. Devolve l->estado.
int  partes_resultado(PartesLista *l);
// O mesmo, so se a ultima busca for DESTA fonte (PARTES_VAZIO se nao for).
int  partes_resultado_de(const Stream *fonte, PartesLista *l);
void partes_cancelar(void);
// A folha de Fontes abriu: le a lista de torrents da conta em segundo plano,
// para o segurar responder da memoria (nada sem chave do TorBox).
void partes_aquecer(void);

// ---- link de um arquivo (fio proprio) -------------------------------------
// Pede o link do CDN de um arquivo. Com `comLista`, le tambem o item (para o
// Retomar, que so tem os ids guardados) e confere que o arquivo ainda existe.
void partes_link_pedir(int usenet, int item, int arquivo, int comLista);
// 0 esperando, 1 pronto (url copiada; a lista, com comLista, em *l), -1 falhou,
// -2 nada pedido. Quem le o 1 ou o -1 consome o resultado.
int  partes_link_estado(char *url, unsigned n, PartesLista *l);
void partes_link_cancelar(void);

// ---- sessao de partes (fio do desenho) ------------------------------------
void partes_sessao_iniciar(const char *alvo, const PartesLista *l, int k);
void partes_sessao_parar(const char *porque);
int  partes_sessao_de(const char *alvo);
const PartesLista *partes_sessao_lista(void);
int  partes_sessao_indice(void);
void partes_sessao_ir(int k);
// Posicao do filme inteiro (ver partes_conta_virtual) quando ha sessao para
// `alvo`. *ultima = esta e a ultima parte.
int  partes_virtual(const char *alvo, double pos, double dur, double catDur,
                    double *vpos, double *vdur, int *ultima);

// ---- o ponto para o Retomar (disco, por conta e perfil) -------------------
typedef struct { int usenet, item, arquivo; double pos; } PartesPonto;
void partes_ponto_guardar(const char *alvo, const char *conta, int perfil, const PartesPonto *p);
int  partes_ponto_pegar(const char *alvo, const char *conta, int perfil, PartesPonto *p);
void partes_ponto_esquecer(const char *alvo, const char *conta, int perfil);
void partes_ponto_zerar(void);   // testes: o proximo acesso rele o disco

// 1 onde o gesto existe (so o .tpk, por enquanto).
int  partes_disponivel(void);

// A FOLHA DE FONTES (streams.c): o arquivo escolhido na modal "Arquivos desta
// fonte". 1 uma vez por escolha: `fonte` e a linha da folha, `l` a lista e
// `k` o arquivo. 0 sem escolha, ou se a lista de fontes mudou por baixo.
int  stream_partes_escolheu(int *fonte, PartesLista *l, int *k);

#endif
