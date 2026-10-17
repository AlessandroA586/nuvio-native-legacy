// THE SAME RELEASE ON ANY EPISODE (owner, 15/10/2026).
//
// Pick a release by hand on one episode, then open ANY other episode of the
// show (earlier or later, from the episode list): the same release must be
// tried first. Before the release fingerprint, the remembered key was only the
// show name (fileKey cuts at the episode marker), so with a broad bingeGroup
// ("aio|1080p") every release of the show tied and the first in the addon's
// list won. Also covers: no match -> -1 (the automatic pick takes over), the
// audio guard, the automatic memory, the four Playback settings, and ids that
// are not IMDb (anime by kitsu id) surviving a cold start.
// Same harness as tests/fontepref_auto.c (NUVIO_DADOS temporary).
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/fontepref.c"

#include "dados.h"

static int falhas;
#define CONFERE(cond, ...) do { \
    if (cond) { printf("ok    "); } else { printf("FALHA "); falhas++; } \
    printf(__VA_ARGS__); printf("\n"); } while (0)

static void fonte(Stream *s, const char *prov, const char *binge, const char *file, int altura) {
  memset(s, 0, sizeof *s);
  snprintf(s->provedor, sizeof s->provedor, "%s", prov);
  snprintf(s->rotulo, sizeof s->rotulo, "%s %dp", prov, altura);
  snprintf(s->url, sizeof s->url, "https://exemplo.invalido/%s", file);
  snprintf(s->bingeGroup, sizeof s->bingeGroup, "%s", binge);
  snprintf(s->arquivo, sizeof s->arquivo, "%s", file);
  s->altura = altura;
  s->mp4 = 0;
}

// Four releases of one show from ONE addon, all in the SAME broad bingeGroup
// and with the same (absent) audio marks: only the release name differs. The
// order changes per episode, as addons reorder by seeders/size.
static const char *GRUPOS[4][2] = {
  { "NTb",    "WEB-DL.DDP5.1.H.264" },
  { "FLUX",   "WEB-DL.DDP5.1.H.265" },
  { "SPARKS", "BluRay.x264" },
  { "GRP",    "WEBRip.x264" },
};
static int lista(Stream *v, int ep, const int *ordem) {
  char f[200];
  int i;
  for (i = 0; i < 4; i++) {
    int g = ordem[i];
    snprintf(f, sizeof f, "The.Show.S01E%02d.1080p.%s-%s.mkv", ep, GRUPOS[g][1], GRUPOS[g][0]);
    fonte(&v[i], "AIOStreams", "aio|1080p", f, 1080);
  }
  return 4;
}
static int posDe(const Stream *v, int n, const char *grupo) {
  int i;
  for (i = 0; i < n; i++) if (strstr(v[i].arquivo, grupo)) return i;
  return -1;
}

int main(void) {
  const char *dir = getenv("NUVIO_DADOS");
  Stream v[6];
  char l[FONTEPREF_LANC];
  int n, k, ep;
  static const int ORDENS[4][4] = { { 0, 1, 2, 3 }, { 3, 2, 0, 1 }, { 2, 0, 3, 1 }, { 0, 3, 1, 2 } };

  if (!dir || !dir[0]) { printf("fontepref_lancamento: NUVIO_DADOS ausente; recusando\n"); return 2; }
  dados_iniciar(dir);
  if (strcmp(dados_dir(), dir)) { printf("fontepref_lancamento: dados_dir diverge; recusando\n"); return 2; }

  // ---- 1. the fingerprint itself
#define LANC(file, esperado) do { Stream s_; fonte(&s_, "A", "", file, 1080); \
    fontepref_lancamento(&s_, l, sizeof l); \
    CONFERE(!strcmp(l, esperado), "lancamento(%s) = %s (esperado %s)", file, l, esperado); } while (0)
  LANC("The.Show.S01E05.1080p.WEB-DL.DDP5.1.H.264-NTb.mkv", "ntb|1080|webdl|avc|");
  LANC("Show.S01E05.2160p.BluRay.REMUX.HEVC.DV.TrueHD.Atmos-FraMeSToR.mkv", "framestor|2160|remux|hevc|dv");
  LANC("[SubsPlease] Show - 05 (1080p) [ABCD1234].mkv", "subsplease|1080|||");
  LANC("Movie.2023.1080p.BluRay.x264-SPARKS.mkv", "sparks|1080|bluray|avc|");
  LANC("4K.Show.S01E01.720p.HDTV.x264-GRP.mkv", "grp|720|hdtv|avc|");
  LANC("Show.S02E01.1080p.WEB-DL.mkv", "|1080|webdl||");
  LANC("Show.S01E05/Show.S01E05.2160p.WEB-DL.HDR10+.HEVC-GRP.mkv", "grp|2160|webdl|hevc|hdr10p");
  LANC("Show.S01E05.Some.Title.mkv", "");
  // The episode marker cuts the show away: two episodes agree.
  { Stream a, b; char la[FONTEPREF_LANC], lb[FONTEPREF_LANC];
    fonte(&a, "A", "", "The.Show.S01E02.1080p.WEB-DL.H.265-FLUX.mkv", 1080);
    fonte(&b, "A", "", "The.Show.S01E09.1080p.WEB-DL.H.265-FLUX.mkv", 1080);
    fontepref_lancamento(&a, la, sizeof la); fontepref_lancamento(&b, lb, sizeof lb);
    CONFERE(!strcmp(la, lb), "E02 e E09 do mesmo lancamento: mesma impressao (%s)", la); }

  // ---- 2. hand pick on E7 -> E2, E5, E9 and E1 pick the SAME release
  fontepref_definir_perfil(1);
  n = lista(v, 7, ORDENS[0]);
  stream_definir_lista(v, n);
  k = posDe(v, n, "FLUX");
  CONFERE(fontepref_guardar("tt4000001:1:7", &v[k]) == 1, "E7: FLUX escolhido a mao");
  for (ep = 0; ep < 4; ep++) {
    static const int EPS[4] = { 2, 5, 9, 1 };
    n = lista(v, EPS[ep], ORDENS[ep]);
    stream_definir_lista(v, n);
    k = fontepref_escolher("tt4000001:1:x");
    CONFERE(k >= 0 && strstr(v[k].arquivo, "-FLUX"), "E%d: o FLUX e o escolhido (posicao %d, ordem %d)",
            EPS[ep], k, ep);
  }
  // Before the fingerprint: in ORDENS[1] FLUX is last; the old rule took the first.
  // The release also survives a cold start (ninth field on disk).
  nTab = 0; nAuto = 0; carregado = 0;
  n = lista(v, 3, ORDENS[1]);
  stream_definir_lista(v, n);
  k = fontepref_escolher("tt4000001:1:3");
  CONFERE(k == 3 && strstr(v[k].arquivo, "-FLUX"), "depois do arranque frio: FLUX (%d)", k);

  // ---- 3. the release is missing in this episode: the bingeGroup still counts
  { static const int SEM_FLUX[4] = { 3, 2, 0, 0 };
    n = lista(v, 10, SEM_FLUX);
    stream_definir_lista(v, n);
    k = fontepref_escolher("tt4000001:1:10");
    CONFERE(k == 0, "sem o FLUX: cai no mesmo bingeGroup, primeira da lista (%d)", k); }
  // ... and nothing of it at all: -1, the automatic pick takes over.
  fonte(&v[0], "Torrentio", "torrentio|1080p", "The.Show.S01E11.1080p.WEB-DL.H.264-NTb.mkv", 1080);
  fonte(&v[1], "Comet", "comet|x", "The.Show.S01E11.1080p.WEB-DL.H.265-FLUX.mkv", 1080);
  stream_definir_lista(v, 2);
  k = fontepref_escolher("tt4000001:1:11");
  CONFERE(k == -1, "outro addon, outro grupo: -1, o automatico assume (%d)", k);

  // ---- 4. a release match never pulls the wrong audio
  fontepref_definir_perfil(2);
  fonte(&v[0], "AIOStreams", "aio|a", "The.Show.S01E01.1080p.WEB-DL.H.265-FLUX.mkv", 1080);
  snprintf(v[0].descricao, sizeof v[0].descricao, "Dublado");
  stream_definir_lista(v, 1);
  CONFERE(fontepref_guardar("tt4000002:1:1", &v[0]) == 1, "DUB FLUX escolhido");
  fonte(&v[0], "AIOStreams", "aio|b", "The.Show.S01E02.1080p.WEB-DL.H.265-FLUX.mkv", 1080);
  stream_definir_lista(v, 1);
  k = fontepref_escolher("tt4000002:1:2");
  CONFERE(k == -1, "mesmo lancamento sem a marca DUB: nao casa (%d)", k);

  // ---- 5. the automatic memory also keeps the release
  fontepref_definir_perfil(3);
  n = lista(v, 1, ORDENS[0]);
  for (k = 0; k < n; k++) v[k].bingeGroup[0] = 0;   // addon without bingeGroup
  stream_definir_lista(v, n);
  CONFERE(fontepref_auto_guardar("tt4000003:1:1", &v[posDe(v, n, "SPARKS")]) == 1,
          "automatico tocou SPARKS (sem bingeGroup): guardado pelo lancamento");
  n = lista(v, 6, ORDENS[2]);
  for (k = 0; k < n; k++) v[k].bingeGroup[0] = 0;
  stream_definir_lista(v, n);
  k = fontepref_escolher("tt4000003:1:6");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-SPARKS"), "E6: SPARKS de novo (%d)", k);
  CONFERE(fontepref_escolher_lista("tt4000003:1:6", v, n, NULL, NULL) == k,
          "a lista propria do proximo episodio concorda (%d)", k);

  // ---- 6. the settings
  fontepref_definir_perfil(1);
  n = lista(v, 4, ORDENS[1]);
  stream_definir_lista(v, n);
  fontepref_configurar(0, 0, 1, 1);
  CONFERE(fontepref_escolher("tt4000001:1:4") == -1 && !fontepref_tem("tt4000001"),
          "Reusar a ultima fonte desligado: a escolha manual nao conta");
  fontepref_configurar(1, 0, 1, 1);
  k = fontepref_escolher("tt4000001:1:4");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-FLUX"), "religado: volta (nada foi apagado) (%d)", k);
  tabela[achar("tt4000001")].quandoS = (long long)time(NULL) - 3LL * 24 * 3600;
  fontepref_configurar(1, 24LL * 3600, 1, 1);
  CONFERE(fontepref_escolher("tt4000001:1:4") == -1, "Lembrar por 1 dia: escolha de 3 dias venceu");
  fontepref_configurar(1, 0, 1, 1);
  CONFERE(fontepref_escolher("tt4000001:1:4") >= 0, "180 dias: vale de novo");
  // Prefer binge group off: the release still counts, the group alone not.
  fontepref_configurar(1, 0, 0, 1);
  k = fontepref_escolher("tt4000001:1:4");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-FLUX"), "sem o grupo: o lancamento ainda casa (%d)", k);
  { static const int SEM_FLUX[4] = { 3, 2, 0, 0 };
    n = lista(v, 12, SEM_FLUX);
    stream_definir_lista(v, n);
    CONFERE(fontepref_escolher("tt4000001:1:12") == 0, "sem o grupo e sem o FLUX: mesmo addon e audio, a primeira"); }
  // ... and the automatic memory is off with it.
  fontepref_definir_perfil(3);
  n = lista(v, 6, ORDENS[2]);
  for (k = 0; k < n; k++) v[k].bingeGroup[0] = 0;
  stream_definir_lista(v, n);
  CONFERE(fontepref_escolher("tt4000003:1:6") == -1, "Preferir o grupo desligado: memoria do automatico parada");
  // Reuse between sessions off: what this process saved counts, a cold start not.
  fontepref_configurar(1, 0, 1, 0);
  k = fontepref_escolher("tt4000003:1:6");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-SPARKS"), "entre sessoes desligado: o desta sessao vale (%d)", k);
  fontepref_definir_perfil(9); fontepref_definir_perfil(3);   // a profile switch rereads the file
  k = fontepref_escolher("tt4000003:1:6");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-SPARKS"), "trocar de perfil nao tira a marca desta sessao (%d)", k);
  nSessao = 0; fontepref_definir_perfil(9); fontepref_definir_perfil(3);   // = a new app start
  CONFERE(fontepref_escolher("tt4000003:1:6") == -1, "e o lido do disco (sessao anterior) nao");
  fontepref_configurar(1, 0, 1, 1);
  k = fontepref_escolher("tt4000003:1:6");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-SPARKS"), "religado: volta (%d)", k);
  // With the manual memory off, a hand-picked source feeds the automatic one.
  fontepref_definir_perfil(4);
  n = lista(v, 1, ORDENS[0]);
  stream_definir_lista(v, n);
  CONFERE(fontepref_guardar("tt4000004:1:1", &v[posDe(v, n, "GRP")]) == 1, "GRP a mao");
  fontepref_configurar(0, 0, 1, 1);
  CONFERE(fontepref_auto_guardar("tt4000004:1:1", &v[posDe(v, n, "GRP")]) == 1,
          "com a manual desligada, a que tocou entra na memoria do automatico");
  n = lista(v, 2, ORDENS[3]);
  stream_definir_lista(v, n);
  k = fontepref_escolher("tt4000004:1:2");
  CONFERE(k >= 0 && strstr(v[k].arquivo, "-GRP"), "e o E2 vai no GRP (%d)", k);
  fontepref_configurar(1, 0, 1, 1);

  // ---- 7. anime by kitsu id survives a cold start
  fontepref_definir_perfil(5);
  fonte(&v[0], "AIOStreams", "", "[SubsPlease] Show - 01 (1080p) [ABCD1234].mkv", 1080);
  fonte(&v[1], "AIOStreams", "", "[Erai-raws] Show - 01 [1080p][Multiple Subtitle].mkv", 1080);
  stream_definir_lista(v, 2);
  CONFERE(fontepref_guardar("kitsu:41370:1", &v[0]) == 1, "kitsu: SubsPlease a mao");
  nTab = 0; nAuto = 0; carregado = 0;
  fonte(&v[0], "AIOStreams", "", "[Erai-raws] Show - 07 [1080p][Multiple Subtitle].mkv", 1080);
  fonte(&v[1], "AIOStreams", "", "[SubsPlease] Show - 07 (1080p) [9F00AA11].mkv", 1080);
  stream_definir_lista(v, 2);
  k = fontepref_escolher("kitsu:41370:7");
  CONFERE(k == 1, "kitsu depois do arranque frio: SubsPlease no ep 7 (%d)", k);

  printf(falhas ? "fontepref_lancamento: %d falha(s)\n" : "fontepref_lancamento: tudo ok\n", falhas);
  return falhas ? 1 : 0;
}
