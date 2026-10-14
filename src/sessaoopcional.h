// Weak DEFAULT definitions of the account-session helpers, for the isolated
// fixtures that omit sessao.c (same idiom as perfisessao.h). A linked sessao.c
// overrides them with its strong definitions. The identity default returns -1,
// a value the real producer never returns, so consumers can tell "no account
// module" from "not signed in". Include only from consumers, never sessao.c.
#ifndef NV_SESSAOOPCIONAL_H
#define NV_SESSAOOPCIONAL_H
#include <stddef.h>
#include "sessao.h"
__attribute__((weak)) int sessao_identidade_local_copiar(char *u, size_t nu, char *j, size_t nj) {
  (void)u; (void)nu; (void)j; (void)nj; return -1;
}
__attribute__((weak)) const char *sessao_usuario(void) { return ""; }
__attribute__((weak)) int sessao_token_copiar(char *t, size_t n) { (void)t; (void)n; return 0; }
__attribute__((weak)) void sessao_token_recusado(const char *t) { (void)t; }
#endif
