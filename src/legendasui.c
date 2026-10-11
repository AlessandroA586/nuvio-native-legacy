// Subtitle selector and second subtitle drawing. See legendasui.h.
#include "legendasui.h"
#include "legenda2.h"
#include "legenda.h"
#include "faixas.h"
#include "idioma.h"
#include "linguas.h"
#include "player.h"
#include "plrui.h"
#include "plrilha.h"
#include "text.h"
#include "ajustes.h"
#include "rede.h"
#include "escala.h"
#include "catalogo.h"
#include "legauto.h"
#include "video.h"
#include "ponteiro.h"
#include "rolagemtoque.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "bidi.h"

// --- identity -------------------------------------------------------------------
// FNV-1a 64 over the fields that identify a candidate within one media session.
// The addon URL takes part (it is what tells two files of the same provider
// apart) but only as hash input: the result is opaque and never reversible to
// the URL, so it can be logged/handed to AutoSync safely.
static unsigned long long fnv(unsigned long long h, const char *s) {
  for (; s && *s; s++) { h ^= (unsigned char)*s; h *= 1099511628211ULL; }
  h ^= 0x1f; h *= 1099511628211ULL;   // field separator
  return h;
}
static void hexId(char tipo, unsigned long long h, char out[24]) {
  snprintf(out, 24, "%c%016llx", tipo, h);
}
void legendasui_id_embutida(const VideoFaixa *f, char out[24]) {
  char n[16];
  // The TV's track number: stable for one media session, unlike the list
  // index (tracks can be re-listed after the MKV probe) and unlike the
  // language (the LG reports "(null)" until the probe fills it).
  snprintf(n, sizeof n, "%d", f ? f->numero : -1);
  hexId('e', fnv(14695981039346656037ULL, n), out);
}
void legendasui_id_addon(const Legenda *l, char out[24]) {
  unsigned long long h = 14695981039346656037ULL;
  h = fnv(h, l ? l->provedor : "");
  h = fnv(h, l ? l->url : "");
  hexId('a', h, out);
}

// Format only when it is actually known: the Matroska CodecID, or the file
// extension in the release name / URL path. Never guessed from the provider.
static void formatoDaExtensao(const char *s, char out[8]) {
  const char *fim, *p;
  char ext[8];
  size_t n, i;
  out[0] = 0;
  if (!s || !*s) return;
  fim = s + strcspn(s, "?#");
  for (p = fim; p > s && p[-1] != '.' && p[-1] != '/'; p--) {}
  if (p == s || p[-1] != '.') return;
  n = (size_t)(fim - p);
  if (n == 0 || n >= sizeof ext) return;
  for (i = 0; i < n; i++) ext[i] = (char)tolower((unsigned char)p[i]);
  ext[n] = 0;
  if (!strcmp(ext, "srt")) snprintf(out, 8, "SRT");
  else if (!strcmp(ext, "vtt")) snprintf(out, 8, "VTT");
  else if (!strcmp(ext, "ass") || !strcmp(ext, "ssa")) snprintf(out, 8, "ASS");
}
static void formatoDoCodec(const char *c, char out[8]) {
  out[0] = 0;
  if (!c || !*c) return;
  if (!strncmp(c, "S_TEXT/ASS", 10) || !strncmp(c, "S_TEXT/SSA", 10)) snprintf(out, 8, "ASS");
  else if (!strncmp(c, "S_TEXT/UTF8", 11)) snprintf(out, 8, "SRT");
  else if (!strncmp(c, "S_TEXT/WEBVTT", 13)) snprintf(out, 8, "VTT");
  else if (!strncmp(c, "S_HDMV/PGS", 10)) snprintf(out, 8, "PGS");
  else if (!strncmp(c, "S_VOBSUB", 8)) snprintf(out, 8, "VobSub");
}

int legendasui_montar(const VideoFaixa *const *emb, int nEmb,
                      const Legenda *add, int nAdd, LegUiCand *out, int max) {
  int n = 0, passo, i;
  // Dialogue tracks first, then signs/forced ones (the order faixas.c uses).
  for (passo = 0; passo < 2; passo++)
    for (i = 0; i < nEmb && n < max; i++) {
      const VideoFaixa *f = emb[i];
      LegUiCand *c;
      if (!f || (f->letreiro ? 1 : 0) != passo) continue;
      c = &out[n++];
      memset(c, 0, sizeof *c);
      legendasui_id_embutida(f, c->id);
      c->embutida = 1; c->indice = i; c->letreiro = f->letreiro;
      // #287: the kind from the track; a forced/signs flag without kind (a
      // platform that only knows the flag) reads as signs, as before.
      c->tipo = f->tipoLeg ? f->tipoLeg : f->letreiro ? LING_LEG_LETREIROS : LING_LEG_COMUM;
      snprintf(c->idioma, sizeof c->idioma, "%s", f->idioma);
      snprintf(c->rotulo, sizeof c->rotulo, "%s", f->rotulo);
      snprintf(c->codec, sizeof c->codec, "%s", f->codec);
      formatoDoCodec(f->codec, c->formato);
    }
  for (i = 0; i < nAdd && n < max; i++) {
    LegUiCand *c = &out[n++];
    memset(c, 0, sizeof *c);
    c->leg = add[i];
    legendasui_id_addon(&add[i], c->id);
    c->indice = i;
    snprintf(c->idioma, sizeof c->idioma, "%s", add[i].idioma);
    snprintf(c->rotulo, sizeof c->rotulo, "%s", add[i].rotulo);
    snprintf(c->origem, sizeof c->origem, "%s", add[i].provedor);
    snprintf(c->arquivo, sizeof c->arquivo, "%s", add[i].arquivo);
    formatoDaExtensao(add[i].arquivo, c->formato);
    if (!c->formato[0]) formatoDaExtensao(add[i].url, c->formato);
  }
  // #287: a plain embedded track next to a forced one of the same language is
  // the full one. Saying so is what tells "Embutida" from "Embutida" apart.
  { int a, b;
    for (a = 0; a < n; a++) {
      if (!out[a].embutida || out[a].tipo != LING_LEG_COMUM) continue;
      for (b = 0; b < n; b++)
        if (out[b].embutida && (out[b].tipo == LING_LEG_FORCADA || out[b].tipo == LING_LEG_LETREIROS) &&
            out[a].idioma[0] && ling_casa(out[b].idioma, out[a].idioma) && ling_casa(out[a].idioma, out[b].idioma))
          out[a].completaPar = 1;
    }
  }
  return n;
}

const char *legendasui_tipo_rotulo(const LegUiCand *c) {
  if (!c || !c->embutida) return NULL;
  if (c->tipo == LING_LEG_COMUM) return c->completaPar ? i18n("Legenda completa") : NULL;
  { const char *r = ling_tipo_legenda_rotulo(c->tipo); return r ? i18n(r) : NULL; }
}

int legendasui_cand_secundaria_ok(const LegUiCand *c) {
  return c && !c->embutida && strcmp(c->formato, "ASS") != 0;
}

// --- the live snapshot -------------------------------------------------------------
static LegUiCand cand[LEGUI_MAX_CAND];
static int nCand;

static int nEmbVivo(void) { int n = video_n_legenda(); return n > NV_FAIXA_MAX ? NV_FAIXA_MAX : n < 0 ? 0 : n; }
static void capturar(void) {
  const VideoFaixa *emb[NV_FAIXA_MAX];
  Legenda *add = NULL;
  int nE = nEmbVivo(), nA = 0, i;
  for (i = 0; i < nE; i++) emb[i] = video_legenda(i);
  // Live channels have no addon subtitles (faixas.c, tests/faixas_canal).
  if (!player_id_canal()[0] && (add = malloc(sizeof *add * LEG_MAX)))
    nA = addons_legendas_copiar(add, LEG_MAX, NULL, NULL);
  nCand = legendasui_montar(emb, nE, add, nA, cand, LEGUI_MAX_CAND);
  free(add);
}

static void idPrimario(char out[24]) {
  const char *x = faixas_legenda_externa_id();
  int i;
  out[0] = 0;
  if (x && x[0]) { snprintf(out, 24, "%s", x); return; }
  i = faixas_legenda_ativa();
  if (i >= 0 && i < nEmbVivo()) legendasui_id_embutida(video_legenda(i), out);
}

const char *legendasui_slot_identidade(int slot) {
  static char p[24];
  if (slot == 1) return legenda2_estado() == LEG2_ATIVA ? legenda2_identidade() : "";
  idPrimario(p);
  return p;
}

static const LegendasSyncProvider *sync;
static LegendasSyncProvider syncCopia;
void legendasui_definir_sync(const LegendasSyncProvider *p) {
  if (p) { syncCopia = *p; sync = &syncCopia; } else sync = NULL;
}

// --- rows --------------------------------------------------------------------------
typedef enum { LR_CAND, LR_NENHUMA, LR_MAIS, LR_SYNC, LR_ALVO, LR_ATRASO } LuiTipo;
typedef struct {
  LuiTipo tipo;
  int slot, cand, nVar;
  char chave[96];
} LuiLinha;
#define LUI_MAX_LINHAS (LEGUI_MAX_CAND * 2 + 8)
static LuiLinha linhas[LUI_MAX_LINHAS];
static int nLinhasV;

static int mais, alvo, aberto;
// #202: a language row with several versions opens the list of THAT group's
// versions (ver = 1). verGrupo is the simple-view row key it came from; Back
// returns focus to it.
static int ver;
static char verGrupo[96];
static char focoChave[96];
static int foco, rolagem, syncAcao;
#ifdef NV_TOUCH_UI
static ToqueRolagem toqueLegendas;
static float toqueLegendasOffset;
static int toqueLegendasRolar(const PonteiroRolagem *e) {
  if (!aberto || faixas_estilo_topo()) return 0;
  return toquerol_evento(&toqueLegendas, e);
}
#endif
static void rolagemZerar(void) {
  rolagem = 0;
#ifdef NV_TOUCH_UI
  toqueLegendasOffset = 0; toquerol_limpar(&toqueLegendas);
#endif
}
static char aviso[128];
static Uint32 avisoAte;

static const char *prefLinha(int slot) {
  const char *p = slot ? ling_legenda2() : ling_legenda();
  if (!p || !strcasecmp(p, "none")) return "";
  return p;
}

// Translated language name. Also part of the row key: the UI language cannot
// change while the player panel is open, so the key stays stable.
static const char *nomeIdioma(const char *cod) { return cod && cod[0] ? i18n(ling_nome(cod)) : "?"; }
static void chaveOrigem(const LegUiCand *c, char *dst, size_t n) {
  // One group per KIND (#287): "Embutida · Forçada" and the full track of the
  // same language are two rows, not "2 versões" (the kind is what the person
  // picks by). Plain stays "E" and signs "E*", as before.
  if (c->embutida) {
    if (c->tipo == LING_LEG_COMUM) snprintf(dst, n, "E");
    else if (c->tipo == LING_LEG_LETREIROS) snprintf(dst, n, "E*");
    else snprintf(dst, n, "E:%c", c->tipo == LING_LEG_FORCADA ? 'F' : c->tipo == LING_LEG_SDH ? 'S' : 'C');
  }
  else snprintf(dst, n, "A:%s", c->origem);
}

// Groups of language + origin for one slot of the simple view. The active
// candidate of that slot always gets its group, filtered or not, so the check
// is never hidden by the filter.
// The SECOND slot of the simple view never offers what would not draw: an
// embedded track or an ASS file of the second language is left out (More
// options still lists it, dimmed, with the reason), and the slot's "Nenhuma"
// row says why (secOculta: 1 = an embedded track, 2 = only ASS files).
static int secOculta;
static void gruposDoSlot(int slot, const char *ativo) {
  const char *pref = prefLinha(slot);
  int i, k;
  if (slot) secOculta = 0;
  int passo, ii;
  // A LINGUA PREFERIDA PRIMEIRO (foto do dono, 04/10: Ingles na frente de
  // Portugues com o idioma em portugues): 3 = o melhor (pt-BR para quem pediu
  // portugues), 2 = a mesma familia, 1 = sem preferencia, 0 = o resto (so a
  // que esta ativa). A ordem de chegada dos addons manda dentro de cada grau.
  for (passo = 3; passo >= 0; passo--)
  for (ii = 0; ii < nCand && nLinhasV < LUI_MAX_LINHAS - 4; ii++) {
    const LegUiCand *c;
    char org[80], chave[96];
    i = ii; c = &cand[i];
    if (ling_afinidade(c->idioma, pref) != passo) continue;
    int casa = slot ? (pref[0] && ling_casa(c->idioma, pref)) : (!pref[0] || ling_casa(c->idioma, pref));
    if (!casa && strcmp(c->id, ativo)) continue;
    if (slot && !legendasui_cand_secundaria_ok(c)) {
      if (c->embutida) secOculta = 1; else if (!secOculta) secOculta = 2;
      continue;
    }
    chaveOrigem(c, org, sizeof org);
    snprintf(chave, sizeof chave, "%c|%s|%s", slot ? 's' : 'p', nomeIdioma(c->idioma), org);
    for (k = 0; k < nLinhasV; k++) if (!strcmp(linhas[k].chave, chave)) break;
    if (k < nLinhasV) {
      linhas[k].nVar++;
      if (!strcmp(c->id, ativo)) linhas[k].cand = i;   // the row stands for the active variant
      continue;
    }
    linhas[nLinhasV].tipo = LR_CAND; linhas[nLinhasV].slot = slot;
    linhas[nLinhasV].cand = i; linhas[nLinhasV].nVar = 1;
    snprintf(linhas[nLinhasV].chave, sizeof linhas[nLinhasV].chave, "%s", chave);
    nLinhasV++;
  }
}

static void linhaFixa(LuiTipo t, int slot, const char *chave) {
  if (nLinhasV >= LUI_MAX_LINHAS) return;
  linhas[nLinhasV].tipo = t; linhas[nLinhasV].slot = slot;
  linhas[nLinhasV].cand = -1; linhas[nLinhasV].nVar = 0;
  snprintf(linhas[nLinhasV].chave, sizeof linhas[nLinhasV].chave, "%s", chave);
  nLinhasV++;
}

static const char *estadoSync(int slot) {
  const char *s = sync && sync->estado ? sync->estado(slot, sync->u) : NULL;
  return s && s[0] ? s : NULL;
}

// Does this candidate belong to the simple-view group `grupo` (same key as
// gruposDoSlot builds)? The second slot never lists what it cannot draw.
static int candNoGrupo(const char *grupo, const LegUiCand *c) {
  char org[80], chave[96];
  int slot = grupo[0] == 's';
  if (slot && !legendasui_cand_secundaria_ok(c)) return 0;
  chaveOrigem(c, org, sizeof org);
  snprintf(chave, sizeof chave, "%c|%s|%s", slot ? 's' : 'p', nomeIdioma(c->idioma), org);
  return !strcmp(chave, grupo);
}

// The version the app would pick on its own: the file whose name shares the
// most words with what is playing; ties (and embedded tracks) keep the first.
static int versaoRecomendada(const char *grupo) {
  const char *midia = video_url_atual();
  int i, melhor = -1, ms = -1;
  for (i = 0; i < nCand; i++) {
    int s;
    if (!candNoGrupo(grupo, &cand[i])) continue;
    s = cand[i].embutida ? 0 : legauto_afinidade_release(cand[i].arquivo, midia ? midia : "");
    if (s > ms) { ms = s; melhor = i; }
  }
  return melhor;
}

static void montarLinhas(void) {
  char prim[24];
  const char *sec = legenda2_identidade();
  int i, slot;
  capturar();
  idPrimario(prim);
  nLinhasV = 0;
  if (ver) {
    int rec = versaoRecomendada(verGrupo);
    for (i = 0; i < nCand && nLinhasV < LUI_MAX_LINHAS; i++) {
      char k[96];
      if (!candNoGrupo(verGrupo, &cand[i])) continue;
      snprintf(k, sizeof k, "v|%s", cand[i].id);
      linhaFixa(LR_CAND, verGrupo[0] == 's', k);
      linhas[nLinhasV - 1].cand = i; linhas[nLinhasV - 1].nVar = i == rec ? -1 : 1;   // -1: recommended
    }
  } else if (!mais) {
    gruposDoSlot(0, prim);
    linhaFixa(LR_NENHUMA, 0, "p|-");
    gruposDoSlot(1, sec);
    linhaFixa(LR_NENHUMA, 1, "s|-");
    for (slot = 0; slot < 2; slot++)
      if (estadoSync(slot)) linhaFixa(LR_SYNC, slot, slot ? "sync|s" : "sync|p");
    linhaFixa(LR_MAIS, 0, "mais");
  } else {
    linhaFixa(LR_ALVO, alvo, "m|alvo");
    linhaFixa(LR_ATRASO, alvo, "m|atraso");
    linhaFixa(LR_NENHUMA, alvo, "m|-");
    for (i = 0; i < nCand && nLinhasV < LUI_MAX_LINHAS; i++) {
      char k[96];
      snprintf(k, sizeof k, "m|%s", cand[i].id);
      linhaFixa(LR_CAND, alvo, k);
      linhas[nLinhasV - 1].cand = i; linhas[nLinhasV - 1].nVar = 1;
    }
  }
  // FOCUS BY IDENTITY: find the row with the remembered key; if it vanished
  // (a list was replaced), stay at the same position and adopt that row.
  for (i = 0; i < nLinhasV; i++) if (!strcmp(linhas[i].chave, focoChave)) break;
  if (i < nLinhasV) foco = i;
  else {
    if (foco >= nLinhasV) foco = nLinhasV - 1;
    if (foco < 0) foco = 0;
    if (nLinhasV) snprintf(focoChave, sizeof focoChave, "%s", linhas[foco].chave);
  }
}

static void focar(int i) {
  if (i < 0 || i >= nLinhasV) return;
  foco = i; syncAcao = 0;
  snprintf(focoChave, sizeof focoChave, "%s", linhas[i].chave);
}

static void avisar(const char *s) {
  snprintf(aviso, sizeof aviso, "%s", s);
  avisoAte = SDL_GetTicks() + 2600u;
}

void legendasui_abrir(void) {
  char prim[24];
  int i;
  aberto = 1; mais = 0; ver = 0; alvo = 0; rolagemZerar(); aviso[0] = 0;
  focoChave[0] = 0; foco = 0;
  montarLinhas();
  // Open on the row of the active primary, else on the first row.
  idPrimario(prim);
  for (i = 0; i < nLinhasV; i++)
    if (linhas[i].tipo == LR_CAND && linhas[i].slot == 0 && linhas[i].cand >= 0 &&
        !strcmp(cand[linhas[i].cand].id, prim)) break;
  if (i == nLinhasV && !prim[0])
    for (i = 0; i < nLinhasV; i++) if (linhas[i].tipo == LR_NENHUMA && linhas[i].slot == 0) break;
  focar(i < nLinhasV ? i : 0);
}

int legendasui_mais(void) { return aberto && mais; }
int legendasui_versoes(void) { return aberto && ver; }

void legendasui_reiniciar(void) {
  legenda2_reiniciar();
  mais = 0; ver = 0; alvo = 0; focoChave[0] = 0; foco = 0; rolagemZerar(); aviso[0] = 0;
}

// --- choosing ----------------------------------------------------------------------
// Re-resolves an embedded candidate against the LIVE track list by identity:
// the snapshot index may have shifted if tracks were re-listed meanwhile.
static int embutidaViva(const LegUiCand *c) {
  int i, n = nEmbVivo();
  char id[24];
  for (i = 0; i < n; i++) {
    legendasui_id_embutida(video_legenda(i), id);
    if (!strcmp(id, c->id)) return i;
  }
  return -1;
}

// OK SEGURADO (#370). O firmware da TV entrega a tecla segurada como KEYDOWNs
// SEPARADOS, cada um com repeat=0 (ver app.c, alternarSalvos): sem esta guarda
// cada um deles recarregava a MESMA legenda (download, SubRip, libass e o plano
// do AutoSync desde o zero) — no log do #370, 9 recargas em 2,3 s. O relogio
// anda a cada toque, entao uma segurada inteira e UMA escolha; soltar e apertar
// de novo (retentar a mesma legenda) passa.
#define LU_REPETE_MS 600u
static int repeticao(int slot, const LegUiCand *c) {
  static char ultId[24]; static int ultSlot = -1; static Uint32 ultMs;
  Uint32 agora = SDL_GetTicks();
  int igual = ultSlot == slot && !strcmp(ultId, c->id) && (Sint32)(agora - ultMs) < (Sint32)LU_REPETE_MS && (Sint32)(agora - ultMs) >= 0;
  ultSlot = slot; ultMs = agora;
  snprintf(ultId, sizeof ultId, "%s", c->id);
  return igual;
}

static void escolher(int slot, const LegUiCand *c) {
  char prim[24];
  if (c && repeticao(slot, c)) return;
  if (slot == 0) {
    if (!c) { faixas_escolher_embutida(-1); return; }
    if (c->embutida) { int i = embutidaViva(c); if (i >= 0) faixas_escolher_embutida(i); }
    else faixas_escolher_externa(&c->leg);
    return;
  }
  if (!c) { legenda2_desligar(); return; }
  if (c->embutida) { avisar(i18n("Embutida como secundária: indisponível")); return; }
  if (!legendasui_cand_secundaria_ok(c)) { avisar(i18n("ASS como secundária: indisponível")); return; }
  if (!(rede_pedido_capacidades() & REDE_CAP_CORPO)) {
    avisar(i18n("Secundária indisponível nesta plataforma"));
    return;
  }
  idPrimario(prim);
  if (!strcmp(prim, c->id)) { avisar(i18n("Já é a principal")); return; }
  legenda2_escolher(c->id, c->leg.url, c->idioma, c->origem);
}

static int ativoNoSlot(int slot, const LegUiCand *c) {
  char prim[24];
  if (!c) return 0;
  if (slot) return !strcmp(legenda2_identidade(), c->id);
  idPrimario(prim);
  return !strcmp(prim, c->id);
}

// ATRASO EM REGUA (pedido de usuario, 05/10): era um botao de 0,25 s por
// toque. Agora cada toque anda 0,1 s e SEGURAR a seta anda 0,5 s por
// repeticao; a regua mostra onde o valor esta entre -10 e +10 s.
#define LU_ATRASO_MAX 10000
static void mudarAtraso(int passo) {
  if (alvo == 0) {
    VideoLegendaEstilo *e = player_leg_estilo();
    e->atrasoMs += passo;
    if (e->atrasoMs > LU_ATRASO_MAX) e->atrasoMs = LU_ATRASO_MAX;
    if (e->atrasoMs < -LU_ATRASO_MAX) e->atrasoMs = -LU_ATRASO_MAX;
    player_leg_estilo_mudou();
  } else {
    int v = legenda2_offset_manual() + passo;
    legenda2_definir_offset_manual(v > LU_ATRASO_MAX ? LU_ATRASO_MAX : v < -LU_ATRASO_MAX ? -LU_ATRASO_MAX : v);
  }
}

static void ajustarRolagem(int vis) {
  if (vis < 1) return;
  if (foco < rolagem) rolagem = foco;
  else if (foco >= rolagem + vis) rolagem = foco - vis + 1;
  if (rolagem > nLinhasV - vis) rolagem = nLinhasV - vis;
  if (rolagem < 0) rolagem = 0;
}

static int nAcoes(int slot, const char **rot, int max) {
  int n = sync && sync->acoes ? sync->acoes(slot, rot, max, sync->u) : 0;
  return n < 0 ? 0 : n > max ? max : n;
}

int legendasui_evento(const SDL_Event *e) {
#ifdef NV_TOUCH_UI
  if (toquerol_navegacao(e)) {
    rolagem = (int)(toqueLegendasOffset / 92.0f);
    toquerol_limpar(&toqueLegendas);
  }
#endif
  SDL_Keycode k;
  LuiLinha *l;
  if (!aberto || e->type != SDL_KEYDOWN) return LEGUI_NADA;
  montarLinhas();
  k = e->key.keysym.sym;
  l = nLinhasV ? &linhas[foco] : NULL;
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE) {
    if (ver) { ver = 0; rolagemZerar(); snprintf(focoChave, sizeof focoChave, "%s", verGrupo); montarLinhas(); return LEGUI_TRATADO; }
    if (mais) { mais = 0; snprintf(focoChave, sizeof focoChave, "mais"); rolagemZerar(); montarLinhas(); return LEGUI_TRATADO; }
    aberto = 0;
    return LEGUI_FECHAR;
  }
  if (k == SDLK_UP)   { if (foco > 0) focar(foco - 1); return LEGUI_TRATADO; }
  if (k == SDLK_DOWN) { if (foco < nLinhasV - 1) focar(foco + 1); return LEGUI_TRATADO; }
  if (k == SDLK_LEFT || k == SDLK_RIGHT) {
    int d = k == SDLK_RIGHT ? 1 : -1;
    if (l && l->tipo == LR_ALVO) { alvo = d > 0; return LEGUI_TRATADO; }
    if (l && l->tipo == LR_ATRASO) { mudarAtraso((e->key.repeat ? 500 : 100) * d); return LEGUI_TRATADO; }
    if (l && l->tipo == LR_SYNC) {
      const char *rot[4];
      int n = nAcoes(l->slot, rot, 4);
      if (n > 0 && syncAcao + d >= 0 && syncAcao + d < n) { syncAcao += d; return LEGUI_TRATADO; }
    }
    if (d > 0) return LEGUI_ESTILO;
    return LEGUI_TRATADO;
  }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    if (!l) return LEGUI_TRATADO;
    switch (l->tipo) {
      case LR_MAIS:
        mais = 1; alvo = 0; rolagemZerar(); focoChave[0] = 0; foco = 0;
        montarLinhas(); focar(2 < nLinhasV ? 3 < nLinhasV ? 3 : 2 : 0);
        break;
      case LR_NENHUMA: escolher(l->slot, NULL); break;
      case LR_CAND:
        if (!mais && !ver && l->nVar > 1) {
          snprintf(verGrupo, sizeof verGrupo, "%s", l->chave);
          ver = 1; rolagemZerar(); focoChave[0] = 0; foco = 0;
          montarLinhas();
          { int i, a = -1;   // open on the version in use, else the recommended one
            for (i = 0; i < nLinhasV; i++) {
              if (linhas[i].cand >= 0 && ativoNoSlot(linhas[i].slot, &cand[linhas[i].cand])) { a = i; break; }
              if (a < 0 && linhas[i].nVar < 0) a = i;
            }
            focar(a >= 0 ? a : 0); }
          break;
        }
        escolher(l->slot, l->cand >= 0 ? &cand[l->cand] : NULL); break;
      case LR_ALVO: alvo = !alvo; break;
      case LR_ATRASO: break;
      case LR_SYNC: {
        const char *rot[4];
        int n = nAcoes(l->slot, rot, 4);
        if (n > 0 && sync && sync->executar) sync->executar(l->slot, syncAcao < n ? syncAcao : 0, sync->u);
        break; }
    }
    return LEGUI_TRATADO;
  }
  return LEGUI_NADA;
}

// --- drawing the panel ---------------------------------------------------------------
#define LU_PAD_X   18.0f
#define LU_PAD_Y   22.0f
#define LU_TIT_H   64.0f
#define LU_LN_H    88.0f
#define LU_LN_VAO   4.0f
#define LU_PE_H    47.0f
// PONTEIRO (#99). A linha sob o cursor ganha o foco pela mesma focar() das
// setas (idempotente: a linha ja focada nao zera a acao da sincronia); o OK do
// clique segue por legendasui_evento. O que o OK nao faz e ESQUERDA/DIREITA
// faz vira clique proprio: os segmentos Principal/Secundaria do "Usar como",
// os discos < > da acao da sincronia, a regua do Atraso (metade esquerda
// adianta, direita atrasa: o passo de 0,1 s da seta) e a aba "Estilo".
// `pont` = a ilha esta assentada neste quadro (alvos so entao).
static int pont;
static int ponteiroAqui(int i) {
  return aberto && !faixas_estilo_topo() && i >= 0 && i < nLinhasV;
}
static void ponteiroLinha(int i, int b) {
  (void)b;
  if (!ponteiroAqui(i) || i == foco) return;
  focar(i);
}
static void ponteiroAlvoSeg(int i, int seg) {
  if (!ponteiroAqui(i) || linhas[i].tipo != LR_ALVO) return;
  ponteiroLinha(i, 0);
  alvo = seg ? 1 : 0;
}
static void ponteiroSyncPasso(int i, int d) {
  const char *rot[4];
  int n;
  if (!ponteiroAqui(i) || linhas[i].tipo != LR_SYNC) return;
  ponteiroLinha(i, 0);
  n = nAcoes(linhas[i].slot, rot, 4);
  if (syncAcao + d >= 0 && syncAcao + d < n) syncAcao += d;
}
// `cx` = o meio da regua na tela real (a do ponteiro).
static void ponteiroAtraso(int i, int cx) {
  if (!ponteiroAqui(i) || linhas[i].tipo != LR_ATRASO) return;
  ponteiroLinha(i, 0);
  mudarAtraso(ponteiro_x() < (float)cx ? -100 : 100);
}
static void ponteiroAbaEstilo(int a, int b) { (void)a; (void)b; if (aberto) faixas_aba_estilo(1); }

int legendasui_teste_foco(int *alvoOut, int *syncOut) {
  if (alvoOut) *alvoOut = alvo;
  if (syncOut) *syncOut = syncAcao;
  return aberto ? foco : -1;
}

static int luVis(void) {
  int n = (int)((NV_VTELA_H - 48.0f - 40.0f - 60.0f - 183.0f + 4.0f) / (LU_LN_H + LU_LN_VAO));
  return n > 7 ? 7 : n < 2 ? 2 : n;
}

float legendasui_altura(void) {
  int n;
  montarLinhas();
  n = nLinhasV < luVis() ? nLinhasV : luVis();
  return LU_PAD_Y + LU_TIT_H + 14.0f + n * LU_LN_H + (n > 0 ? (n - 1) * LU_LN_VAO : 0.0f) +
         14.0f + LU_PE_H + LU_PAD_Y;
}

static void rosto(GfxRect r, const char *idioma, const char *icone, int sel, float a) {
  if (ajustes_vidro()) gfx_cor(r, 16.0f / r.h, 1, 1, 1, (sel ? 0.12f : 0.07f) * a);
  else if (sel) gfx_cor(r, 16.0f / r.h, 0.22f, 0.227f, 0.259f, a);
  else gfx_cor(r, 16.0f / r.h, 0.125f, 0.129f, 0.153f, a);
  if (icone) {
    gfx_icone((GfxRect){ r.x + 15.0f, r.y + 15.0f, 22.0f, 22.0f }, icone, 1, 1, 1, (sel ? 1.0f : 0.7f) * a);
  } else if (idioma && idioma[0]) {
    // "PT-BR", "PT", "EN": a familia, e nao as 2 primeiras letras do codigo ("por" virava "PO").
    const char *c = ling_selo(idioma);
    int k = sel ? 255 : 178;
    TxtLinha l = txt_linha(strlen(c) > 3 ? TXT_ILHA_APOIO : TXT_G16B, c, k, k, k, 255);
    txt_desenhar_alpha(l, r.x + (r.w - (float)l.w) * 0.5f, r.y + (r.h - (float)l.h) * 0.5f, a);
  }
}

// Slot marker pill at the right edge; returns its width.
static float pilulaSlot(int slot, float xDir, float yc, int sel, float a) {
  TxtLinha t = txt_linha(TXT_ILHA_APOIO, slot ? "Secundária" : "Principal", 243, 242, 239, sel ? 230 : 150);
  float w = (float)t.w + 24.0f, h = (float)t.h + 10.0f;
  GfxRect r = { xDir - w, yc - h * 0.5f, w, h };
  gfx_cor(r, 0.5f, 1, 1, 1, (slot ? 0.06f : 0.10f) * a);
  txt_desenhar_alpha(t, r.x + 12.0f, yc - (float)t.h * 0.5f, a);
  return w;
}

static void juntar(char *dst, size_t n, const char *parte) {
  size_t l = strlen(dst);
  if (!parte || !parte[0] || l + 1 >= n) return;
  snprintf(dst + l, n - l, "%s%s", l ? "  \xc2\xb7  " : "", parte);
}

// State of the second subtitle for the row that holds the active selection.
static const char *estadoSecundaria(void) {
  switch (legenda2_estado()) {
    case LEG2_CARREGANDO:    return i18n("Carregando…");
    case LEG2_FALHOU:        return i18n("Não carregou");
    case LEG2_NAO_SUPORTADA: return i18n("ASS como secundária: indisponível");
    default:                 return NULL;
  }
}

static void textoLinha(const LuiLinha *l, char *nome, size_t tn, char *sub, size_t ts,
                       const char **idioma, const char **icone, int *ativo, int *apagada) {
  const LegUiCand *c = l->cand >= 0 ? &cand[l->cand] : NULL;
  nome[0] = sub[0] = 0; *idioma = NULL; *icone = NULL; *ativo = 0; *apagada = 0;
  switch (l->tipo) {
    case LR_NENHUMA:
      // A segunda lista tem a SUA "nenhuma": com o mesmo nome da primeira as
      // duas pareciam repetidas (foto do dono, 04/10).
      snprintf(nome, tn, "%s", i18n(!mais && l->slot == 1 ? "Sem segunda legenda" : "Nenhuma"));
      if (!mais && l->slot == 1 && secOculta)
        snprintf(sub, ts, "%s", secOculta == 1 ? i18n("Embutida como secundária: indisponível")
                                               : i18n("ASS como secundária: indisponível"));
      else snprintf(sub, ts, "%s", i18n("Sem legenda"));
      *icone = "pl_eye-off";
      *ativo = l->slot ? !legenda2_identidade()[0] : !legendasui_slot_identidade(0)[0];
      break;
    case LR_MAIS:
      snprintf(nome, tn, "%s", i18n("Mais opções"));
      snprintf(sub, ts, "%s", i18n("Todas as legendas, com detalhes"));
      *icone = "pl_layers";
      break;
    case LR_SYNC: {
      // Title + the provider's state below; the chosen action is drawn
      // between < > on the right when the row has focus (desenharLinha).
      const char *s = estadoSync(l->slot);
      snprintf(nome, tn, "%s", i18n("Sincronização automática"));
      snprintf(sub, ts, "%s", s ? s : "");
      *icone = "aj_wand";
      break; }
    case LR_ALVO:
      snprintf(nome, tn, "%s", i18n("Usar como"));
      *icone = "pl_captions";
      break;
    case LR_ATRASO: {
      int v = l->slot ? legenda2_offset_manual() : player_leg_estilo()->atrasoMs;
      snprintf(nome, tn, "%s", i18n("Atraso"));
      if (!v) snprintf(sub, ts, "0 s");
      else { snprintf(sub, ts, "%+.2f s", v / 1000.0f); plrui_decimal(sub); }
      *icone = "aj_clock";
      break; }
    case LR_CAND:
      if (!c) break;
      *idioma = c->idioma;
      *ativo = ativoNoSlot(l->slot, c) && (l->slot == 0 || legenda2_estado() == LEG2_ATIVA);
      if (!mais && !ver) {
        snprintf(nome, tn, "%s", nomeIdioma(c->idioma));
        juntar(sub, ts, c->embutida ? i18n("Embutida") : c->origem);
        juntar(sub, ts, legendasui_tipo_rotulo(c));
        if (l->nVar > 1) { char b[32]; snprintf(b, sizeof b, i18n("%d versões"), l->nVar); juntar(sub, ts, b); }
      } else {
        const char *marca = c->embutida ? faixas_legenda_marca(c->indice) : NULL;
        snprintf(nome, tn, "%s", c->embutida ? c->rotulo : nomeIdioma(c->idioma));
        // The version the app would pick by itself: marked on the title line,
        // which has the full width (the details line is busy with the file name).
        if (l->nVar < 0) juntar(nome, tn, i18n("Recomendada"));
        juntar(sub, ts, c->embutida ? i18n("Embutida") : c->origem);
        juntar(sub, ts, c->formato);
        { const char *t = legendasui_tipo_rotulo(c);
          if (t && !strstr(nome, t)) juntar(sub, ts, t); }   // the title may already say it
        juntar(sub, ts, c->arquivo);
        if (l->slot == 0 && marca) juntar(sub, ts, marca);
      }
      if (l->slot == 1 && !legendasui_cand_secundaria_ok(c)) {
        sub[0] = 0;
        juntar(sub, ts, c->embutida ? i18n("Embutida como secundária: indisponível")
                                    : i18n("ASS como secundária: indisponível"));
        *apagada = 1;
      } else if (l->slot == 1 && ativoNoSlot(1, c) && estadoSecundaria()) {
        char b[160];
        snprintf(b, sizeof b, "%s", estadoSecundaria());
        sub[0] = 0; juntar(sub, ts, b);
      }
      break;
  }
}

static void desenharLinha(const LuiLinha *l, int idx, int sel, float x, float y, float w, float a) {
  char nome[96], sub[256];
  const char *idioma, *icone;
  int ativo, apagada;
  float dir = x + w - 22.0f, aL = a;
  GfxRect lr = { x, y, w, LU_LN_H };
  textoLinha(l, nome, sizeof nome, sub, sizeof sub, &idioma, &icone, &ativo, &apagada);
  if (apagada) aL = a * 0.55f;
  if (sel) plrui_linha_foco(lr, 22.0f, a);
  if (pont) ponteiro_alvo(x, y, w, LU_LN_H, ponteiroLinha, NULL, idx, 0);
  rosto((GfxRect){ x + 22.0f, y + (LU_LN_H - 52.0f) * 0.5f, 52.0f, 52.0f }, idioma, icone, sel, aL);
  if (ativo) {
    float ar, ag, ab;
    ajustes_acento(&ar, &ag, &ab);
    gfx_icone((GfxRect){ dir - 28.0f, y + (LU_LN_H - 28.0f) * 0.5f, 28.0f, 28.0f }, "pl_check", ar, ag, ab, a);
    dir -= 28.0f + 12.0f;
  }
  if (l->tipo == LR_ALVO) {
    const char *rot[2] = { "Principal", "Secundária" };
    int cont[2] = { -1, -1 };
    float sw = plrui_seg(rot, cont, 2, alvo, sel, -1.0f, 0, a);
    plrui_seg(rot, cont, 2, alvo, sel, dir - sw, y + (LU_LN_H - 54.0f) * 0.5f, a);
    if (pont) {   // o primeiro segmento mede o que mede o segmentado de um so
      float w0 = plrui_seg(rot, cont, 1, alvo, sel, -1.0f, 0, a);
      ponteiro_alvo(dir - sw, y + (LU_LN_H - 54.0f) * 0.5f, w0, 54.0f, ponteiroLinha, ponteiroAlvoSeg, idx, 0);
      ponteiro_alvo(dir - sw + w0, y + (LU_LN_H - 54.0f) * 0.5f, sw - w0, 54.0f, ponteiroLinha, ponteiroAlvoSeg, idx, 1);
    }
    dir -= sw + 12.0f;
  } else if (!mais && !ver && (l->tipo == LR_CAND || l->tipo == LR_NENHUMA)) {
    // (No pill on the AutoSync row: only the primary syncs, and the title
    // needs the width next to the < action > discs.)
    // In "Mais opções" the slot is the "Usar como" row at the top: no pill per
    // row, so the details (release name) get the width.
    dir -= pilulaSlot(l->slot, dir, y + LU_LN_H * 0.5f, sel, aL) + 12.0f;
    // Several versions: OK opens them, and the chevron says so.
    if (l->tipo == LR_CAND && l->nVar > 1) {
      gfx_icone((GfxRect){ dir - 24.0f, y + (LU_LN_H - 24.0f) * 0.5f, 24.0f, 24.0f }, "pl_chevron-right", 1, 1, 1, (sel ? 0.9f : 0.5f) * aL);
      dir -= 24.0f + 10.0f;
    }
  }
  if (l->tipo == LR_SYNC && sel) {
    // The chosen action between < > discs; a disc dims with nothing beyond it.
    const char *rot[4];
    int n = nAcoes(l->slot, rot, 4);
    if (n == 1) {
      // R4: a unica acao (Desfazer) e um botao, nao um seletor com setas.
      TxtLinha t = txt_linha(TXT_G16B, rot[0], 243, 242, 239, 255);
      float cy = y + LU_LN_H * 0.5f, bw = (float)t.w + 28.0f;
      dir -= bw;
      gfx_cor((GfxRect){ dir, cy - 15.0f, bw, 30.0f }, 0.5f, 1, 1, 1, 0.10f * a);
      txt_desenhar_alpha(t, dir + 14.0f, cy - (float)t.h * 0.5f, a);
      dir -= 12.0f;
    } else if (n > 1) {
      int k = syncAcao < n ? syncAcao : 0;
      TxtLinha t = txt_linha(TXT_G16B, rot[k], 243, 242, 239, 255);
      float cy = y + LU_LN_H * 0.5f, dw = 30.0f, ad = k < n - 1 ? 1.0f : 0.3f, ae = k > 0 ? 1.0f : 0.3f;
      dir -= dw; gfx_cor((GfxRect){ dir, cy - 15.0f, dw, 30.0f }, 0.5f, 1, 1, 1, 0.10f * ad * a);
      gfx_icone((GfxRect){ dir + 6.0f, cy - 9.0f, 18.0f, 18.0f }, "pl_chevron-right", 1, 1, 1, ad * a);
      if (pont) ponteiro_alvo(dir - 6.0f, cy - 21.0f, dw + 12.0f, 42.0f, ponteiroLinha, ponteiroSyncPasso, idx, 1);
      dir -= 10.0f + (float)t.w;
      txt_desenhar_alpha(t, dir, cy - (float)t.h * 0.5f, a);
      dir -= 10.0f + dw; gfx_cor((GfxRect){ dir, cy - 15.0f, dw, 30.0f }, 0.5f, 1, 1, 1, 0.10f * ae * a);
      gfx_icone((GfxRect){ dir + 6.0f, cy - 9.0f, 18.0f, 18.0f }, "pl_chevron-left", 1, 1, 1, ae * a);
      if (pont) ponteiro_alvo(dir - 6.0f, cy - 21.0f, dw + 12.0f, 42.0f, ponteiroLinha, ponteiroSyncPasso, idx, -1);
      dir -= 12.0f;
    }
  }
  if (l->tipo == LR_ATRASO) {
    // Regua: trilho de -10 a +10 s, marca no zero, preenchido do zero ate o
    // valor na cor de destaque; o botao so aparece com a linha em foco.
    int v = l->slot ? legenda2_offset_manual() : player_leg_estilo()->atrasoMs;
    float tw = 260.0f, tx = dir - tw - 6.0f, cy = y + LU_LN_H * 0.5f;
    float k = (float)v / (float)LU_ATRASO_MAX, px, ar, ag, ab, al = (sel ? 1.0f : 0.55f) * a;
    k = k < -1.0f ? -1.0f : k > 1.0f ? 1.0f : k;
    px = tx + tw * 0.5f + k * tw * 0.5f;
    ajustes_acento_marca(&ar, &ag, &ab);
    gfx_cor((GfxRect){ tx, cy - 3.0f, tw, 6.0f }, 0.5f, 1, 1, 1, 0.14f * al);
    if (v) gfx_cor((GfxRect){ v > 0 ? tx + tw * 0.5f : px, cy - 3.0f, fabsf(px - (tx + tw * 0.5f)), 6.0f }, 0.5f, ar, ag, ab, al);
    gfx_cor((GfxRect){ tx + tw * 0.5f - 1.0f, cy - 9.0f, 2.0f, 18.0f }, 0, 1, 1, 1, 0.38f * al);
    if (sel) {
      gfx_cor((GfxRect){ px - 12.0f, cy - 12.0f, 24.0f, 24.0f }, 0.5f, 0.055f, 0.059f, 0.071f, a);
      gfx_cor((GfxRect){ px - 9.0f, cy - 9.0f, 18.0f, 18.0f }, 0.5f, 0.953f, 0.949f, 0.937f, a);
    } else gfx_cor((GfxRect){ px - 6.0f, cy - 6.0f, 12.0f, 12.0f }, 0.5f, 0.953f, 0.949f, 0.937f, al);
    if (pont) ponteiro_alvo(tx - 12.0f, y, tw + 24.0f, LU_LN_H, ponteiroLinha, ponteiroAtraso, idx,
                            (int)((tx + tw * 0.5f) * gfx_escala()));
    dir = tx - 18.0f;
  }
  { float tx = x + 22.0f + 52.0f + 18.0f, tw = dir - tx;
    int c = sel ? 255 : 219, cs = 122;
    TxtLinha ln = txt_linha_corta(TXT_ILHA_FORTE, nome, c, c, c - 2, 255, tw);
    TxtLinha ls = txt_linha_corta(TXT_ILHA_GENERO, sub, cs, cs, cs - 2, 255, tw);
    float th = (float)ln.h + (sub[0] ? 5.0f + (float)ls.h : 0.0f), ty = y + (LU_LN_H - th) * 0.5f;
    txt_desenhar_alpha(ln, tx, ty, aL);
    if (sub[0]) txt_desenhar_alpha(ls, tx, ty + (float)ln.h + 5.0f, aL); }
}

static const char *tituloSessao(void) {
  const CatItem *c = cat_item(player_indice());
  return c && c->titulo[0] ? c->titulo : "";
}

void legendasui_corpo(GfxRect c, float a) {
  float x0 = c.x + LU_PAD_X, w = c.w - LU_PAD_X * 2.0f, y = c.y + LU_PAD_Y;
  int vis = luVis(), fim, i;
  montarLinhas();
  pont = a > 0.99f && ponteiro_ativo();
  plrui_kicker(tituloSessao(), x0 + 10.0f, y, 115, 115, 113, a);
  { TxtLinha t = txt_linha(TXT_ILHA_PERGUNTA, mais ? "Mais opções" : ver && nLinhasV && linhas[0].cand >= 0 ? nomeIdioma(cand[linhas[0].cand].idioma) : "Legendas", 243, 242, 239, 255);
    float ty = y + 22.0f;
    const char *rot[2] = { "Faixas", "Estilo" };
    int cont[2] = { nCand, -1 };
    float sw = plrui_seg(rot, cont, 2, 0, 0, -1.0f, 0, a);
    txt_desenhar_alpha(t, x0 + 10.0f, ty, a);
    plrui_seg(rot, cont, 2, 0, 0, x0 + w - 10.0f - sw, ty + (float)t.h - 54.0f, a);
    if (pont) {   // a aba "Estilo": o que sobra depois do primeiro segmento
      float w0 = plrui_seg(rot, cont, 1, 0, 0, -1.0f, 0, a);
      ponteiro_alvo(x0 + w - 10.0f - sw + w0, ty + (float)t.h - 54.0f, sw - w0, 54.0f, NULL, ponteiroAbaEstilo, 0, 0);
    } }
  y += LU_TIT_H + 14.0f;
#ifdef NV_TOUCH_UI
  if (!toqueLegendas.livre) {
#endif
  ajustarRolagem(vis);
#ifdef NV_TOUCH_UI
    toqueLegendasOffset = rolagem * (LU_LN_H + LU_LN_VAO);
  }
  { float passo = LU_LN_H + LU_LN_VAO;
    int n = nLinhasV < vis ? nLinhasV : vis;
    float area = n * passo - (n > 0 ? LU_LN_VAO : 0);
    GfxRect ilha;
    int ptr = pont;
    toquerol_vincular(&toqueLegendas, (GfxRect){x0, y, w, area}, gfx_escala(),
                     0, nLinhasV * passo - LU_LN_VAO - area, 1, &toqueLegendasOffset);
    if (aberto && a > 0.99f) ponteiro_rolagem(toqueLegendasRolar);
    gfx_recorte(x0, y, w, area);
    for (i = 0; i < nLinhasV; i++) {
      float ry = y + i * passo - toqueLegendasOffset;
      if (ry + LU_LN_H < y || ry > y + area) continue;
      pont = ptr && ry >= y && ry + LU_LN_H <= y + area;
      desenharLinha(&linhas[i], i, i == foco, x0, ry, w, a);
      if (ptr && !pont) ponteiro_alvo_faixa(x0, ry, w, LU_LN_H, y, y + area, ponteiroLinha, NULL, i, 0);
    }
    pont = ptr;
    if (plrilha_rect(&ilha)) gfx_recorte(ilha.x, ilha.y, ilha.w, ilha.h);
    else gfx_recorte(c.x, c.y, c.w, c.h);
    y += area + 14.0f;
  }
#else
  fim = rolagem + vis; if (fim > nLinhasV) fim = nLinhasV;
  for (i = rolagem; i < fim; i++)
    desenharLinha(&linhas[i], i, i == foco, x0, y + (i - rolagem) * (LU_LN_H + LU_LN_VAO), w, a);
  y += (fim - rolagem) * LU_LN_H + (fim - rolagem > 0 ? (fim - rolagem - 1) * LU_LN_VAO : 0.0f) + 14.0f;
#endif
  gfx_cor((GfxRect){ x0, y, w, 1.0f }, 0.0f, 1, 1, 1, 0.07f * a);
  { float yc = y + 16.0f + 15.0f;
    if (aviso[0] && (Sint32)(avisoAte - SDL_GetTicks()) > 0) {
      TxtLinha l = txt_linha_corta(TXT_ILHA_APOIO, aviso, 255, 214, 120, 255, w - 20.0f);
      txt_desenhar_alpha(l, x0 + 10.0f, yc - (float)l.h * 0.5f, a);
    } else {
      char q[32];
      const char *k[3] = { "OK", "\xe2\x86\x92", "Voltar" };
      const char *rt[3] = { "Usar", "Estilo", mais || ver ? "Voltar" : "Fechar" };
      int naSync = nLinhasV && linhas[foco].tipo == LR_SYNC, nf = 0, pf = 0;
      // The AutoSync row is not a choice: it is left out of "N de M", which
      // disappears on it, and the hints say what OK does there.
      for (i = 0; i < nLinhasV; i++) if (linhas[i].tipo != LR_SYNC) { nf++; if (i <= foco) pf = nf; }
      if (!naSync) {
        snprintf(q, sizeof q, i18n("%d de %d"), nf ? pf : 0, nf);
        { TxtLinha l = txt_linha(TXT_ILHA_APOIO, q, 243, 242, 239, 115);
          txt_desenhar_alpha(l, x0 + 10.0f, yc - (float)l.h * 0.5f, a); }
        plrui_dicas(k, rt, 3, x0 + w - 10.0f, yc, 1, a);
      } else {
        const char *rot[4], *ks[3] = { "OK", "\xe2\x86\x90 \xe2\x86\x92", "Voltar" };
        int n = nAcoes(linhas[foco].slot, rot, 4);
        const char *rs[3] = { n ? rot[syncAcao < n ? syncAcao : 0] : "", "Op\xc3\xa7\xc3\xa3o", "Fechar" };
        if (n == 1) { const char *k2[2] = { "OK", "Voltar" }, *r2[2] = { rot[0], "Fechar" };
          plrui_dicas(k2, r2, 2, x0 + w - 10.0f, yc, 1, a); }
        else if (n) plrui_dicas(ks, rs, 3, x0 + w - 10.0f, yc, 1, a);
        else plrui_dicas(ks + 2, rs + 2, 1, x0 + w - 10.0f, yc, 1, a);
      }
    } }
}

// --- the second subtitle on screen ---------------------------------------------------
// TOP BAND. The primary keeps the bottom (where the person's position setting
// puts it, above the controls and the next-episode card). The second subtitle
// takes the top of the picture, the convention of mpv's secondary-sid: it
// never competes with a multi-line primary, with the progress bar, with the
// skip/next-episode buttons or with the OSD, which all live at the bottom.
// What does live at the top is the clock island (and the format badges next
// to it while the OSD is up): the band moves below them while the controls
// are up and squeezes to the side of a grown island (the subtitle panel).
#define LEG2_LINHAS 4
static float bandaFim;
static float larguraSecundaria(void) { return NV_TELA_W - 120.0f; }

static void corEstilo(int i, int *r, int *g, int *b) {
  static const unsigned char c[VIDEO_LEG_NCORES][3] = {
    {255,255,255},{255,222,48},{64,224,112},{78,156,255},{255,80,80},{18,18,18}};
  if (i < 0 || i >= VIDEO_LEG_NCORES) i = 0;
  *r = c[i][0]; *g = c[i][1]; *b = c[i][2];
}

typedef struct { TxtLinha cor, borda; } Leg2Linha;

// Arabic/Hebrew: shape + visual order of an already wrapped line (bidi.c),
// in place. Latin text is left byte-identical.
static void visualLinha(TxtFamilia fam, TxtEstilo est, char *linha) {
  char v[768];
  if (txt_bidi_legenda(fam, est, linha, v, sizeof v) > 0) memcpy(linha, v, strlen(v) + 1);
}

static int quebrar(const LegendaCue *c, TxtEstilo est, int r, int g, int b, int borda,
                   float maxW, Leg2Linha *out, int max) {
  char texto[768], *linha, *salva;
  TxtFamilia fam = (TxtFamilia)player_leg_estilo()->familia;
  int enf = (c->negrito || player_leg_estilo()->negrito ? TXT_ENF_NEGRITO : 0) | (c->italico ? TXT_ENF_ITALICO : 0);
  int n = 0;
  snprintf(texto, sizeof texto, "%s", c->texto);
  for (linha = strtok_r(texto, "\n", &salva); linha && n < max; linha = strtok_r(NULL, "\n", &salva)) {
    char atual[768] = "", tent[768], *pal, *sp;
    for (pal = strtok_r(linha, " ", &sp); pal; pal = strtok_r(NULL, " ", &sp)) {
      snprintf(tent, sizeof tent, "%s%s%s", atual, atual[0] ? " " : "", pal);
      if (atual[0] && txt_linha_corta_enfase(est, tent, r, g, b, 255, 1e9f, fam, enf).w > maxW) {
        if (n >= max) break;
        visualLinha(fam, est, atual);
        out[n].cor = txt_linha_corta_enfase(est, atual, r, g, b, 255, maxW, fam, enf);
        out[n].borda = borda ? txt_linha_corta_enfase(est, atual, 0, 0, 0, 255, maxW, fam, enf) : (TxtLinha){0};
        n++;
        snprintf(atual, sizeof atual, "%s", pal);
      } else snprintf(atual, sizeof atual, "%s", tent);
    }
    if (atual[0] && n < max) {
      visualLinha(fam, est, atual);
      out[n].cor = txt_linha_corta_enfase(est, atual, r, g, b, 255, maxW, fam, enf);
      out[n].borda = borda ? txt_linha_corta_enfase(est, atual, 0, 0, 0, 255, maxW, fam, enf) : (TxtLinha){0};
      n++;
    }
  }
  return n;
}

void legendasui_banda_zerar(void) { bandaFim = 0.0f; }

// Estilo proprio da segunda legenda (Ajustes): cada campo "igual a principal"
// (0 / -1) segue o estilo da principal, como era antes.
static void estiloSecundario(const VideoLegendaEstilo *e, TxtEstilo *est, int *r, int *g, int *b,
                             int *fundo, int *borda) {
  int pct = ajustes_leg2_tamanho(), c = ajustes_leg2_cor(), f = ajustes_leg2_fundo(), bd = ajustes_leg2_borda();
  if (pct <= 0) pct = e->tamanho * 9 / 10;
  if (pct < 50) pct = 50;
  if (pct > 250) pct = 250;
  pct = (pct / 10) * 10;
  *est = (TxtEstilo)(TXT_LEG_50 + (pct - 50) / 10);
  corEstilo(c >= 0 ? c : e->cor, r, g, b);
  *fundo = f >= 0 ? f : e->fundo;
  *borda = bd >= 0 ? bd : e->borda;
}

void legendasui_desenhar_secundaria(const LegendasGeo *g) {
  LegendaCue cues[LEGENDA_SIMULTANEAS];
  Leg2Linha ln[LEG2_LINHAS];
  const VideoLegendaEstilo *e = player_leg_estilo();
  int n, i, nl = 0, r, gg, b, fundo, borda;
  float topo, x0 = 60.0f, x1 = 60.0f + larguraSecundaria(), alpha, y, altura = 0.0f;
  TxtEstilo est;
  GfxRect il;
  bandaFim = 0.0f;
  // Minimised player / window animation: the second line has nowhere honest
  // to go, and the primary still identifies the scene.
  if (!g || g->videoW < NV_TELA_W * 0.6f) return;
  n = legenda2_cues(g->pos, cues, LEGENDA_SIMULTANEAS);
  if (n <= 0) return;
  estiloSecundario(e, &est, &r, &gg, &b, &fundo, &borda);
  alpha = (e->opacidade == 3 ? .25f : e->opacidade == 2 ? .5f : e->opacidade == 1 ? .75f : 1.f) * g->alpha;
  topo = 48.0f + 72.0f * (g->osd < 0 ? 0 : g->osd > 1 ? 1 : g->osd);
  if (g->videoY > topo) topo = g->videoY + 24.0f;   // letterbox: start on the picture
  if (!g->junto && plrilha_rect(&il)) {
    float s = gfx_escala_ui();
    il.x *= s; il.y *= s; il.w *= s; il.h *= s;
    if (il.y < topo + 260.0f && il.y + il.h > topo) {
      if (il.y + il.h + 16.0f < topo + 120.0f) topo = il.y + il.h + 16.0f;   // small pill: go under it
      else if (il.x + il.w * 0.5f > NV_TELA_W * 0.5f) x1 = il.x - 32.0f;
      else x0 = il.x + il.w + 32.0f;
      if (x1 - x0 < 560.0f) { x0 = 60.0f; x1 = 60.0f + larguraSecundaria(); topo = il.y + il.h + 16.0f; }
    }
  }
  for (i = 0; i < n && nl < LEG2_LINHAS; i++)
    nl += quebrar(&cues[i], est, r, gg, b, borda, x1 - x0, ln + nl, LEG2_LINHAS - nl);
  if (g->junto) {
    // Stacked: the whole block ends just above the primary (2-line cues grow
    // upward, never over it) and stays inside the picture.
    for (i = 0; i < nl; i++) altura += (float)ln[i].cor.h + (i ? 5.0f : 0.0f);
    topo = g->baseY - 10.0f - altura;
    if (topo < 8.0f) topo = 8.0f;
  }
  y = topo;
  for (i = 0; i < nl; i++) {
    TxtLinha l = ln[i].cor;
    float x = (x0 + x1) * 0.5f - (float)l.w * 0.5f;
    if (fundo) gfx_cor((GfxRect){ x - 18, y - 6, (float)l.w + 36, (float)l.h + 12 }, .16f, 0, 0, 0, fundo * .16f * alpha);
    if (ln[i].borda.tex) {
      float d = borda == 2 ? 4.f : 2.f;
      txt_desenhar_alpha(ln[i].borda, x + d, y + d, .82f * alpha);
      if (borda == 1) {
        txt_desenhar_alpha(ln[i].borda, x - d, y, .82f * alpha);
        txt_desenhar_alpha(ln[i].borda, x, y - d, .82f * alpha);
      }
    }
    txt_desenhar_alpha(l, x, y, alpha);
    y += (float)l.h + 5.0f;
  }
  if (!g->junto) bandaFim = y;
}

// Altura do bloco da segunda legenda agora (0 = nada a mostrar). O player usa
// para abrir lugar EMBAIXO da principal no modo "Junto da principal".
float legendasui_altura_secundaria(const LegendasGeo *g) {
  LegendaCue cues[LEGENDA_SIMULTANEAS];
  Leg2Linha ln[LEG2_LINHAS];
  const VideoLegendaEstilo *e = player_leg_estilo();
  int n, i, nl = 0, r, gg, b, fundo, borda;
  float altura = 0.0f;
  TxtEstilo est;
  if (!g || g->videoW < NV_TELA_W * 0.6f) return 0.0f;
  n = legenda2_cues(g->pos, cues, LEGENDA_SIMULTANEAS);
  if (n <= 0) return 0.0f;
  estiloSecundario(e, &est, &r, &gg, &b, &fundo, &borda);
  for (i = 0; i < n && nl < LEG2_LINHAS; i++)
    nl += quebrar(&cues[i], est, r, gg, b, borda, larguraSecundaria(), ln + nl, LEG2_LINHAS - nl);
  for (i = 0; i < nl; i++) altura += (float)ln[i].cor.h + (i ? 5.0f : 0.0f);
  return altura;
}

float legendasui_topo_livre(float minimo) {
  return bandaFim > 0.0f && bandaFim + 12.0f > minimo ? bandaFim + 12.0f : minimo;
}
