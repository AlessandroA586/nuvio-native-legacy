// watch_state v2: a ponta com rede e disco. O contrato (corpo, id, fila, retry)
// vive em watchstate.c e e testado sem rede; aqui so ha o diario em disco e um
// fio de fundo que entrega. Quem toca o video nunca espera por ele.
#ifndef NV_WATCHSTATE_ENVIO_H
#define NV_WATCHSTATE_ENVIO_H

#include "watchstate.h"

// Le o diario e acorda o fio para o que ficou pendente da sessao anterior.
// Idempotente; chamar no arranque (e o primeiro wsenvio_emitir tambem chama).
void wsenvio_iniciar(void);

// Entrega `e` a todo addon instalado cujo manifesto pede esse evento. Volta na
// hora: so enfileira. Nao faz nada sem rede com POST (WGT), em perfil que usa os
// addons de outro, nem para evento invalido.
void wsenvio_emitir(const WsEvento *e);

#endif
