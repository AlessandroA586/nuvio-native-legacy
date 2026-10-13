// watch_state v2 (AIOStreams): o player EMPURRA eventos de reproducao para os
// addons instalados que declaram o resource "watch_state". Este modulo e
// o contrato, sem rede nem SDL: le o manifesto, filtra, monta corpo e id.
// Fila/retry ficam em watchstate_fila (mesmo .c, abaixo). Spec local:
// .mercado/aiostreams-watch-state.md, plano em docs/issues/PLANO-MERCADO.md (W1-W3).
#ifndef NV_WATCHSTATE_H
#define NV_WATCHSTATE_H

#include <stddef.h>

enum { WS_START = 1, WS_PAUSE = 2, WS_STOP = 4, WS_PLAYED = 8, WS_UNPLAYED = 16,
       WS_TODOS = 31 };

#define WS_PREF_MAX 12
#define WS_PREF_TAM 24

// O que o manifesto de UM addon declara. Zerado = nao participa.
typedef struct {
  int  push;                 // 1 = resource watch_state + watchState.version 2 + push{}
  int  eventos;              // mascara WS_* que o addon quer (so reproducao)
  int  viewers;              // watchState.viewers (adiado: so 0 e empurrado, ver watchstate_aceita)
  char tipos[64];            // "|movie|series|" minusculo; vazio = qualquer tipo
  char pref[WS_PREF_MAX][WS_PREF_TAM]; int nPref;   // vazio = qualquer id
} WsDecl;

// Le o manifesto `corpo`. Sempre zera `d` antes; sem resource watch_state, sem
// watchState.push ou com version diferente de 2 deixa push = 0.
void ws_decl_ler(const char *corpo, WsDecl *d);

// O addon quer o evento `ev` (WS_*) de um video do tipo `tipo` ("movie" |
// "series"), de meta `metaId` e video `videoId`? Os idPrefixes casam com
// QUALQUER dos dois ids (W9: meta IMDb com videos Kitsu).
int ws_decl_aceita(const WsDecl *d, int ev, const char *tipo,
                   const char *metaId, const char *videoId);

#endif
