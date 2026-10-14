// Optional linkage for the existing isolated module fixtures. Every complete
// app links perfis.c; fixtures that omit account infrastructure retain their
// original behaviour. Tests of profile ownership must link the real module.
#ifndef NV_PERFISESSAO_H
#define NV_PERFISESSAO_H
#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
// The real producers live in perfis.c, which isolated fixtures may omit (an
// undefined weak_import is a hard error in a plain Mach-O static link, so an
// import-side test cannot work there). Weak DEFAULT definitions below supply
// the no-profile behaviour — generation 0 — when perfis.c is absent; a linked
// perfis.c overrides them, because a strong definition beats a weak one at
// link time and duplicate weak definitions coalesce. perfis.c never includes
// this header, so a translation unit never sees both.
__attribute__((weak)) uint64_t perfis_sessao_geracao(void) { return 0; }
__attribute__((weak)) int perfis_sessao_atual(uint64_t geracao) { return geracao == 0; }
__attribute__((weak)) int perfis_sessao_travar(uint64_t geracao) { return perfis_sessao_atual(geracao); }
__attribute__((weak)) void perfis_sessao_destravar(void) {}
static inline uint64_t nv_perfil_sessao_ler(void) { return perfis_sessao_geracao(); }
static inline int nv_perfil_sessao_atual(uint64_t g) { return perfis_sessao_atual(g); }
static inline int nv_perfil_sessao_travar(uint64_t g) { return perfis_sessao_travar(g); }
static inline void nv_perfil_sessao_destravar(void) { perfis_sessao_destravar(); }
#else
#include "perfis.h"
#define nv_perfil_sessao_ler perfis_sessao_geracao
#define nv_perfil_sessao_atual perfis_sessao_atual
#define nv_perfil_sessao_travar perfis_sessao_travar
#define nv_perfil_sessao_destravar perfis_sessao_destravar
#endif
typedef uint64_t NvPerfilSessao;
#endif
