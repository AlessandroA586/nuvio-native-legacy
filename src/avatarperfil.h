// O AVATAR DE UM PERFIL, num circulo: a foto da conta quando ela tem uma, e
// senao o disco na cor do perfil com a inicial (o que o app web mostra quando
// `avatar_url` e nulo). Um desenho so, para o rodape do menu, a Central de
// controle e quem mais precisar de um perfil pequeno.
//
// A foto e pedida pela largura com que desenha (tex_obter_larg): o teto unico
// de 640 decodificava ate 1,6 MB de avatar para um circulo de 44.
#ifndef NV_AVATARPERFIL_H
#define NV_AVATARPERFIL_H
#include "gfx.h"
#include "perfis.h"

// `p` NULL desenha o disco azul padrao com "?". A inicial muda de corpo com o
// diametro (menos de 36, menos de 48, o resto).
void avatarperfil_desenhar(GfxRect r, const ContaPerfil *p, float alpha);

#endif
