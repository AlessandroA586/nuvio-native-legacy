// Ver avatarperfil.h.
#include "avatarperfil.h"
#include "tex_cache.h"
#include "text.h"
#include <stdio.h>
#include <string.h>

// Cor do avatar a partir do "#RRGGBB" que a conta guarda. Sem cor legivel, o
// azul do padrao do web.
static void corAvatar(const char *hex, float *r, float *g, float *b) {
  unsigned v = 0;
  *r = 0.12f; *g = 0.53f; *b = 0.90f;
  if (!hex || hex[0] != '#' || strlen(hex) < 7) return;
  if (sscanf(hex + 1, "%6x", &v) != 1) return;
  *r = (float)((v >> 16) & 255) / 255.0f;
  *g = (float)((v >> 8) & 255) / 255.0f;
  *b = (float)(v & 255) / 255.0f;
}

// A INICIAL do nome, respeitando UTF-8: um nome comecado por acento tem dois
// bytes, e cortar no primeiro desenha lixo.
static void inicialDe(const char *nome, char *dst, size_t tam) {
  if (tam < 3) { if (tam) dst[0] = 0; return; }
  dst[0] = (nome && nome[0]) ? nome[0] : '?';
  dst[1] = 0;
  if (nome && (unsigned char)nome[0] >= 0xC0 && nome[1]) { dst[1] = nome[1]; dst[2] = 0; }
}

void avatarperfil_desenhar(GfxRect av, const ContaPerfil *p, float alpha) {
  GLuint tex = (p && p->avatarUrl[0]) ? tex_obter_larg(p->avatarUrl, av.w) : 0;
  if (tex) {
    gfx_tex_aspect_atual = 1.0f;
    gfx_rect(av, tex, GFX_CARD, 0, 0, 0, 0.5f, 0, 0, 0, alpha);
  } else {
    float cr, cg, cb;
    char ini[4];
    TxtEstilo est = av.w < 36.0f ? TXT_MINI : (av.w < 48.0f ? TXT_V2_KBD18 : TXT_ILHA_INICIAL);
    TxtLinha l;
    corAvatar(p ? p->corHex : NULL, &cr, &cg, &cb);
    gfx_cor(av, 0.5f, cr, cg, cb, alpha);
    inicialDe(p ? p->nome : NULL, ini, sizeof ini);
    l = txt_linha(est, ini, 255, 255, 255, 255);
    txt_desenhar_alpha(l, av.x + ((float)av.w - (float)l.w) * 0.5f,
                       av.y + ((float)av.h - (float)l.h) * 0.5f, alpha);
  }
}
