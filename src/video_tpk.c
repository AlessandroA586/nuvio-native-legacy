// Player do .tpk da Samsung. Quem toca e o host .NET (Tizen.Multimedia.Player,
// tizen-tpk/Program.cs), no plano de video da TV, por baixo do GLWindow; aqui
// fica so o estado que o resto do app le (video.h) e as chamadas ao host.
//
// O host registra as funcoes dele uma vez (nv_tpk_video_registrar) e avisa o
// que acontece pelo nv_tpk_video_evento, de qualquer fio. O app le o estado no
// fio dele; por isso os campos sao volatile e nada aqui bloqueia.
//
// Faixas: o host manda a lista depois do prepare (nv_tpk_video_faixa) e o app
// escolhe por hEscolher. Legenda EMBUTIDA: o player entrega o texto por evento
// (SubtitleUpdated, com duracao) e o app desenha, igual ao Tizen web. Legenda
// EXTERNA (OpenSubtitles/addon) nem passa por aqui: legenda.c baixa e desenha.
//
// #269: legenda embutida de TEXTO num MKV (ASS, SRT, WebVTT) nao depende mais
// do SubtitleUpdated: faixas.c entrega a faixa ao overlay do app, que a le do
// MKV por Range (mkvass.c, ligado aqui por mkvass_aceitar_texto), e o player
// fica com a legenda "desligada" (legAtual = -1: o texto dele, se vier, nao e
// desenhado — sem desenho duplo). O caminho nativo continua para fonte que nao
// e MKV, faixa sem par no cabecalho e no-go do mkvass, e agora deixa rastro:
// "[video] tpk: first subtitle cue ..." ou "... no subtitle cue in 60 s ...".
#ifdef NV_TPK
#include "video.h"
#include "video_reconexao.h"
#include "idioma.h"
#include "linguas.h"
#include "mkv.h"
#include "capmkv.h"
#include "mkvass.h"
#include "rede.h"
#include "faixasmkv.h"
#include "velocidade.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef void (*FnAbrir)(const char *url, const char *cabecalhos);
typedef void (*FnSemArg)(void);
typedef void (*FnInt)(int);
typedef void (*FnRet)(int x, int y, int w, int h);
typedef int  (*FnPos)(void);
// Source crop in RATIOS of the frame (0..1), destination in screen units.
// Returns 1 when the TV took it, 0 when this build has no such call (Tizen 4)
// or the call refused the values.
typedef int  (*FnCropSource)(double rx, double ry, double rw, double rh,
                               int dx, int dy, int dw, int dh);

static FnAbrir  hAbrir;
static FnSemArg hParar;
static FnInt    hPausar, hBuscar, hVolume;
static FnRet    hJanela;
static FnPos    hPos;
static FnCropSource hCropSource;
typedef void (*FnEscolher)(int tipo, int idx);
static FnEscolher hEscolher;

#define MAX_FAIXAS 32
static VideoFaixa faixaAudio[MAX_FAIXAS], faixaLeg[MAX_FAIXAS];
static volatile int nAudio, nLeg, audioAtual, legAtual = -1;
static SDL_mutex *travaLeg;
static char legTexto[1024];
static Uint32 legAte;
// Host cue callbacks may run on another thread; only access under travaLeg.
static int legCuesBloqueados = 1;
// #269 DIAGNOSTICO DO SubtitleUpdated. Nenhum log de TV jamais mostrou um cue
// nativo chegando, entao nao se sabia se o evento dispara no Tizen 9. Sob
// travaLeg: `legEnvAtivo`/`legEnvEm` = ha escrita de faixa no player, e
// quando; `legEnvFaixa` = qual; `legCueSessao` = ja veio
// algum cue nesta sessao, `legCueEnv` = ja veio cue depois da escrita.
// `legSemCueMs` (fio do app) = tempo TOCANDO desde a escrita, sem cue.
static Uint32 legEnvEm, legAbriuEm;
static int legEnvAtivo;
static int legEnvFaixa = -1, legCueSessao, legCueEnv, legSemCueLogado;
static Uint32 legSemCueMs, legBombeouEm;
#ifndef LEG_SEM_CUE_MS
#define LEG_SEM_CUE_MS 60000u   // o teste encurta
#endif

static char urlAtual[4096];
static char cabecalhos[2048];
static volatile int ativo, pronto, falhou, terminou, tocando, largura, altura;
static volatile int conflito;   // ver video_tpk_log_host
static volatile int durMs, bufferando;
// VELOCIDADE (#202, video.h). O host aplica por escolher(3, centesimos) —
// Player.SetPlaybackRate — e responde no evento EV_VELOCIDADE (a = centesimos,
// b = 1 aceitou / 0 recusou). A documentacao do Tizen.Multimedia diz que o
// SetPlaybackRate lanca InvalidOperationException em "Streaming playback", e
// toda fonte do app e streaming: a linha aparece, o primeiro pedido e a prova,
// e a recusa a esconde pelo resto da execucao.
static volatile int velPedida = 100, velEnviada = 100, velRecusada;
static volatile Uint32 bufferDesde;
static unsigned sessao;
// The host reports numeric errors from another thread. Preserve that evidence
// for the source failure screen without guessing a codec or network cause.
static atomic_uint erroDetalhe;
static atomic_int temErroDetalhe;
// DEFERRED SELECTION (see escolhasPendentes). The track choice is born too
// soon after open and the write is swallowed: measured on the TV, the subtitle
// write at 0.00 s of the first tick was accepted by the player's bookkeeping
// and NEVER applied to the demuxer. Audio only needed the deferral (measured:
// the Korean audio came up right); the subtitle waits for the player to settle.
// Counted from the first FRAME, not from EV_TOCANDO - see escolhasPendentes.
static volatile int comecou;                 // the FRAME arrived (not EV_TOCANDO)
static Uint32 comecouEm;                     // when the frame arrived (SDL clock)
static volatile int audioComecou;            // audio only needs the first tick
static int audioPend = -1, legPend = -1;     // choice waiting to be written

// RECONEXAO (video_reconexao.h). O evento 5 chega de qualquer fio e so ANOTA;
// a decisao e o recarregar sao do video_bombear. A classe do erro sai do
// codigo do evento ou da linha "erro ConnectionFailed" que o host loga logo
// antes dele (Video.cs, ErrorOccurred).
static NvReconexao recon;
static int reconProxima, reconPermitida, reconIniciou;
static volatile int reconErroPend, reconErroCod, reconLinhaRede;
// Escolhas no instante da queda, e o que falta devolver ao recarregar que abriu.
static int reconAudio = -1, reconLeg = -1;
static volatile int reconFaixasPend;
static int reconBuscarMs = -1;
// #412: relogio congelado com estado Playing, fora de pausa/buffer/seek.
#define TPK_PRESO_MS 15000u
static Uint32 progressoEm;
static double progressoPos;
static int vigiaAtiva, pausaPedida;
static atomic_int vigiaReiniciar;

// CABECALHO DO MKV (#206). O player do host so da o idioma de cada faixa; o
// Name ("Forced", "DD 5.1"), a FlagForced e os canais estao nas TrackEntry, e
// o .wgt ja os mostra por isso. Fonte, nesta ordem: o trecho que a pre-busca
// do mkvass ja leu (sem rede, e o caso comum: no log da 1.6.5 da QE65Q80A o
// cabecalho chegou ANTES das faixas), senao um Range proprio num fio, so com o
// video andando ha 5 s (mesmo gatilho do video_tizen.c). A RECONEXAO reabre o
// MESMO arquivo: o que foi lido continua valendo.
// mkvEstado: 0 nada a fazer, 1 procurando, 2 fio na rede, 3 lido (ou desistiu).
static MkvFaixa mkvFx[MKV_MAX_FAIXAS];
static volatile int mkvN, mkvEstado, faixasNovas;
// #269: a folha pediu a sonda ja (video_sondar_mkv_agora); `mkvNaoMkv`: a
// sonda pela rede voltou sem TrackEntry nenhuma (nao e MKV).
static volatile int mkvSondarJa, mkvNaoMkv;
static unsigned mkvGeracao;
// AUDIO QUE A TV RECUSA (#313). A Samsung nao toca DTS (2018+) nem TrueHD: o
// player so lista as faixas que decodifica, entao um MKV so com DTS chega com a
// lista de audio vazia e o filme roda mudo, sem erro nenhum. O cabecalho do MKV
// (mkvFx) diz o que o arquivo tem; a lista do player (nAudio) diz o que a TV
// aceitou. audioNaoSup = 1 so quando TODA faixa de audio do arquivo e de um
// codec que a TV nao toca E a TV nao listou nenhuma (lista com ao menos uma
// faixa quer dizer que ha audio saindo: nao se avisa). NAO PROVADO em TV: o que
// o Player devolve exatamente nesse caso; a linha [audio] do log e a prova.
static volatile int audioNaoSup, faixasLidas;
static int audioLogado;
static int fonteMp4;
static Uint32 mkvOlhou;

__attribute__((visibility("default")))
void nv_tpk_video_registrar(FnAbrir abrir, FnSemArg parar, FnInt pausar, FnInt buscar,
                            FnInt volume, FnRet janela, FnPos pos) {
  hAbrir = abrir; hParar = parar; hPausar = pausar; hBuscar = buscar;
  hVolume = volume; hJanela = janela; hPos = pos;
}

__attribute__((visibility("default")))
void nv_tpk_video_registrar_faixas(FnEscolher escolher) { hEscolher = escolher; }

// The source crop, registered apart from nv_tpk_video_registrar on purpose: that
// signature is resolved BY NAME by the Tizen 4/5 ELF loader and is called by all
// four hosts, so widening it would touch every host and the loader. A second
// entry point costs nothing and leaves what works alone.
__attribute__((visibility("default")))
void nv_tpk_video_crop_register(FnCropSource r) { hCropSource = r; }

// Whether THIS TV has the source-crop call. The host probes with reflection once
// at startup and reports the answer here, because it decides which aspect modes
// the player may offer at all: with no crop, five of the eight draw the same full
// screen on a 16:9 frame, and the other three are a button that does nothing -
// which is exactly the report this fixes.
static int cropOk;
__attribute__((visibility("default")))
void nv_tpk_video_crop_available(int tem) { cropOk = tem ? 1 : 0; }

void video_escolher_audio(int i);

// Mesma regra do video.c / video_tizen.c: sem preferencia ou sem faixa que
// case, fica a do arquivo.
static void escolherAudioPreferido(void) {
  const char *pref = ling_audio();
  int i;
  if (!pref[0] || nAudio < 2) return;
  if (audioAtual >= 0 && audioAtual < nAudio && faixaAudio[audioAtual].idioma[0] &&
      ling_casa(faixaAudio[audioAtual].idioma, pref)) return;
  for (i = 0; i < nAudio; i++) {
    if (!faixaAudio[i].idioma[0] || !ling_casa(faixaAudio[i].idioma, pref)) continue;
    printf("[video] audio preferido: %s (faixa %d de %d)\n", ling_nome(faixaAudio[i].idioma), i + 1, nAudio);
    video_escolher_audio(i);
    return;
  }
}

// Host, fio principal, depois do prepare: uma chamada por faixa (tipo 0 =
// audio, 1 = legenda) e no fim nv_tpk_video_faixas_fim. O contador so sobe no
// fim, com a lista inteira escrita: o app nunca le faixa pela metade.
static VideoFaixa novasA[MAX_FAIXAS], novasL[MAX_FAIXAS];
static int nNovasA, nNovasL;
__attribute__((visibility("default")))
void nv_tpk_video_faixa(int tipo, int idx, const char *lingua) {
  VideoFaixa *f;
  const char *l = lingua ? lingua : "";
  if (tipo == 0) { if (nNovasA >= MAX_FAIXAS) return; f = &novasA[nNovasA++]; }
  else           { if (nNovasL >= MAX_FAIXAS) return; f = &novasL[nNovasL++]; }
  memset(f, 0, sizeof *f);
  printf("[video] tpk faixa %s%d idioma=%s\n", tipo ? "L" : "A", idx, l[0] ? l : "-");
  f->numero = idx;
  f->ordinalMkv = tipo ? idx : -1;
  if (strcmp(l, "und") && strcmp(l, "unknown")) snprintf(f->idioma, sizeof f->idioma, "%s", l);
  if (f->idioma[0]) snprintf(f->rotulo, sizeof f->rotulo, "%s", i18n(ling_nome(f->idioma)));
  else snprintf(f->rotulo, sizeof f->rotulo, "%s %d", i18n(tipo ? "Legenda" : "Áudio"),
                tipo ? nNovasL : nNovasA);
}
__attribute__((visibility("default")))
void nv_tpk_video_faixas_fim(int selAudio, int selLeg) {
  // Segunda leitura (o host rele o audio com o video ja tocando, #165): so o
  // audio muda; a legenda que o app ja escolheu fica.
  int releitura = (nAudio || nLeg) && !nNovasL && nLeg;
  memcpy(faixaAudio, novasA, sizeof novasA);
  audioAtual = selAudio >= 0 ? selAudio : 0;
  if (!releitura) {
    memcpy(faixaLeg, novasL, sizeof novasL);
    legAtual = -1;   // a TV ate pode ter uma escolhida; quem liga e o app (faixas.c)
    nLeg = nNovasL;
  }
  (void)selLeg;
  nAudio = nNovasA;
  nNovasA = nNovasL = 0;
  faixasNovas = 1;   // video_bombear reaplica o cabecalho do MKV, se ja lido
  faixasLidas = 1;
  audioLogado = 0; audioNaoSup = 0;   // o host rele o audio em 2 s (#165): decide de novo
  printf("[video] faixas: %d audio, %d legenda\n", nAudio, nLeg);
  fflush(stdout);
  // Recarregar de reconexao: as faixas que a pessoa tinha, e nao a preferencia.
  // Sem audio ainda (o host rele em 2 s, #165), o audio fica para a releitura.
  if (reconFaixasPend) {
    if (!releitura && reconLeg >= 0 && reconLeg < nLeg) video_escolher_legenda(reconLeg);
    if (nAudio > 0) {
      reconFaixasPend = 0;
      if (reconAudio > 0 && reconAudio < nAudio) video_escolher_audio(reconAudio);
    }
    printf("[video] reconexao: faixas devolvidas (audio %d, legenda %d)\n", reconAudio, reconLeg);
    fflush(stdout);
    return;
  }
  escolherAudioPreferido();
}

// Host: texto da legenda embutida escolhida, valido por `durMs`.
__attribute__((visibility("default")))
void nv_tpk_video_legenda(const char *texto, int durMs) {
  int primeiroSessao = 0, primeiroEnv = 0, faixa = -1, chars = texto ? (int)strlen(texto) : 0;
  Uint32 agora = SDL_GetTicks(), desdeEnv = 0, desdeAbriu = 0;
  if (!travaLeg) return;
  SDL_LockMutex(travaLeg);
    // O cue que chega DURANTE a escrita (bloqueado) e da faixa velha: nao prova
  // a nova.
  if (!legCueSessao) { legCueSessao = 1; primeiroSessao = 1; desdeAbriu = agora - legAbriuEm; }
  if (legEnvAtivo && !legCueEnv && !legCuesBloqueados) {
    legCueEnv = 1; primeiroEnv = 1; desdeEnv = agora - legEnvEm; faixa = legEnvFaixa;
  }
  SDL_UnlockMutex(travaLeg);
  if (primeiroEnv)
    printf("[video] tpk: first subtitle cue after %u ms, %d chars (track=%d, dur %d ms)\n",
           (unsigned)desdeEnv, chars, faixa, durMs);
  else if (primeiroSessao)
    printf("[video] tpk: first subtitle cue of the session %u ms after open, %d chars (no track written)\n",
           (unsigned)desdeAbriu, chars);
  if (primeiroEnv || primeiroSessao) fflush(stdout);
  SDL_LockMutex(travaLeg);
  if (legCuesBloqueados) { SDL_UnlockMutex(travaLeg); return; }
  snprintf(legTexto, sizeof legTexto, "%s", texto ? texto : "");
  legAte = SDL_GetTicks() + (Uint32)(durMs > 0 ? durMs : 3000);
  SDL_UnlockMutex(travaLeg);
}

enum { EV_PRONTO = 1, EV_TOCANDO = 2, EV_PAUSADO = 3, EV_FIM = 4, EV_ERRO = 5,
       EV_TAMANHO = 6, EV_BUFFER = 7, EV_VELOCIDADE = 8 };

// HAS THE PLAYER EVER REALLY PLAYED IN THIS SESSION? Sticky: EV_TOCANDO is the
// host saying Start() took, and nothing clears it until a new video opens.
// The crop needs exactly this and not more - see escolhasPendentes.

__attribute__((visibility("default")))
void nv_tpk_video_evento(int tipo, int a, int b) {
  if (!ativo) return; // evento tardio depois de sair
  if ((tipo == EV_TOCANDO && !tocando) || tipo == EV_PAUSADO ||
      (tipo == EV_BUFFER && bufferando != (a < 100)))
    atomic_store(&vigiaReiniciar, 1);
  switch (tipo) {
    case EV_PRONTO:  durMs = a; pronto = 1; break;
    case EV_TOCANDO: tocando = 1; bufferando = 0; break;
    case EV_PAUSADO: tocando = 0; break;
    case EV_FIM:     terminou = 1; tocando = 0; break;
    // Sem `falhou` aqui: o video_bombear decide entre reconectar e desistir.
    case EV_ERRO:    atomic_store(&erroDetalhe, (unsigned)a);
                     atomic_store(&temErroDetalhe, 1);
                     reconErroCod = a; reconErroPend = 1; tocando = 0;
                     printf("[video] tpk: player error 0x%08x (%d)\n", (unsigned)a, b); break;
    case EV_TAMANHO: largura = a; altura = b; break;
    case EV_BUFFER:
      if (a < 100 && !bufferando) { bufferando = 1; bufferDesde = SDL_GetTicks(); }
      else if (a >= 100) bufferando = 0;
      break;
    case EV_VELOCIDADE:
      vel_log(a, b ? "plataforma ok" : "plataforma recusou");
      if (!b) { velRecusada = 1; velPedida = 100; }
      break;
    default: break;
  }
  if (tipo != EV_BUFFER) { printf("[video] tpk evento %d (%d, %d)\n", tipo, a, b); fflush(stdout); }
}

static void legendaLimpar(int bloquear) {
  if (!travaLeg) return;
  SDL_LockMutex(travaLeg);
  legTexto[0] = 0; legAte = 0; legCuesBloqueados = bloquear;
  SDL_UnlockMutex(travaLeg);
}
// #269: marca a escrita da faixa `i` no player (ou -1 = nenhuma), para o
// diagnostico do primeiro cue / do silencio de 60 s.
static void legendaMarcarEnvio(int i) {
  if (!travaLeg) return;
  SDL_LockMutex(travaLeg);
  legEnvAtivo = i >= 0; legEnvEm = SDL_GetTicks();
  legEnvFaixa = i; legCueEnv = 0;
  SDL_UnlockMutex(travaLeg);
  legSemCueMs = 0; legSemCueLogado = 0;
}
static void legendaEnviar(int i) {
  // Discard the cached old cue and callbacks during the host write. The host
  // callback has no track/session identity; late cues after dispatch cannot
  // be identified here and still require the host's track switch to work.
  legendaLimpar(1);
  legendaMarcarEnvio(i);
  if (hEscolher) hEscolher(1, faixaLeg[i].numero);
  legendaLimpar(0);
}

// #269: escrita feita, video TOCANDO ha 60 s e nenhum cue do player: o
// SubtitleUpdated nao entrega esta faixa nesta TV. Uma linha por escrita, com
// o codec do cabecalho do MKV (o player nao expoe codec de legenda: a
// SubtitleTrackInfo do Tizen.Multimedia so tem GetCount/GetLanguageCode/
// Selected). Conta tempo de reproducao, nao de relogio: pausa e buffer nao
// valem como silencio.
static void legendaVigiarSilencio(Uint32 agora) {
  int env, cue, faixa, l = legAtual;
  Uint32 dt = legBombeouEm ? agora - legBombeouEm : 0;
  legBombeouEm = agora;
  if (!travaLeg || legSemCueLogado || l < 0) return;
  SDL_LockMutex(travaLeg);
  env = legEnvAtivo; cue = legCueEnv; faixa = legEnvFaixa;
  SDL_UnlockMutex(travaLeg);
  if (!env || cue || faixa != l) return;
  if (tocando && !bufferando && dt < 1000u) legSemCueMs += dt;
  if (legSemCueMs < LEG_SEM_CUE_MS) return;
  legSemCueLogado = 1;
  printf("[video] tpk: no subtitle cue in %u s (track=%d, lang=%s, codec=%s, mkv probe=%d)\n",
         LEG_SEM_CUE_MS / 1000u, faixa, faixaLeg[l].idioma[0] ? faixaLeg[l].idioma : "-",
         faixaLeg[l].codec[0] ? faixaLeg[l].codec : "unknown", mkvEstado);
  fflush(stdout);
}

int  video_iniciar(void) {
  if (!travaLeg) travaLeg = SDL_CreateMutex();
  // #269: legenda de texto simples do MKV pelo overlay (ver faixas.c).
  mkvass_aceitar_texto(1);
  return hAbrir != NULL;
}
int  video_iniciar_auto(void) { return hAbrir != NULL; }
int  video_registro_negado(void) { return 0; }

static int emTrailer = 0;   // ver video_tpk_trailer_marcar

// A CROP CHOSEN BEFORE THERE IS A PICTURE IS LOST, SO IT IS SENT AGAIN.
//
// Seen on 2026-10-10: the first press with the video PAUSED did nothing, it started
// working once the film played, and after that it worked even in pause. That matches
// the measurement already recorded for the trailer on this same set (src/trailer.c):
// asked before playing, the crop is ACCEPTED and IGNORED, and only a LATER request
// lands.
//
// So a crop owed is sent once more when the first frame arrives, which is when the
// plane stops swallowing writes. The mode saved in Ajustes is applied at OPEN,
// inside that window, and that is the case this exists for; a press made while the
// film already runs is sent by the press itself and needs nothing here.
//
// ONE extra send, and not a repeating loop: each send is several calls into the
// player, and the plane is shared with the window positioning player.c drives.
// Hammering it races the two for no gain.
static int cropOwed;                       // the plane still has to be told this
static int owedIdentity;                   // ...and the answer is the WHOLE FRAME
static int owedSrcX, owedSrcY, owedSrcW, owedSrcH;
static int owedDstX, owedDstY, owedDstW, owedDstH;

// A NEW SESSION FORGETS THE OWED CROP. Without this it belonged to the FIRST video
// of the app's life, so every later title carried a stale one, and leaving and
// resuming appeared to be the only fix.
static void cropRearm(void) { cropOwed = 0; owedIdentity = 0; }

// True while a crop this side sent may still be on the plane. Set when the host
// takes a crop, cleared by a whole-frame write that took. Sticky across sessions on
// purpose: a reconnect reuses the player, and the crop outlives the session that
// asked for it. It is what makes video_janela clear only when there is something to
// clear.
static int cropAtivo;

static int cropSource(int qw, int qh, int sx, int sy, int sw, int sh,
                      int dx, int dy, int dw, int dh);
static void cropClear(int dx, int dy, int dw, int dh);

// Sends it again, now that the film is really running.
static void cropSendOwed(void) {
  if (!cropOwed) return;
  cropOwed = 0;
  if (owedIdentity) {
    owedIdentity = 0;
    printf("[video] tpk source crop: re-sending the whole frame now that the film is running\n");
    fflush(stdout);
    cropClear(owedDstX, owedDstY, owedDstW, owedDstH);
    return;
  }
  printf("[video] tpk source crop: sending again now that the film is running\n");
  fflush(stdout);
  cropSource(largura, altura, owedSrcX, owedSrcY, owedSrcW, owedSrcH,
             owedDstX, owedDstY, owedDstW, owedDstH);
}

// Abre urlAtual no host. Serve a fonte nova e ao recarregar da reconexao.
static int abrirSessao(void) {
  // video_iniciar() NAO e chamada no .tpk (so video.c, o ramo da LG, a chama;
  // aqui o trailer usa video_iniciar_auto, que nao a chama), entao a flag do
  // texto simples ficava em 0 e toda faixa S_TEXT/UTF8 saia do mkvass como
  // "no-go: faixa nao e ASS" e voltava para a TV (2.0.1: 13 pessoas, 125 logs,
  // e nenhuma linha "e texto simples"). Ligada a cada sessao, antes de o app
  // escolher qualquer faixa.
  mkvass_aceitar_texto(1);
  atomic_store(&temErroDetalhe, 0);
  vigiaAtiva = pausaPedida = 0;
  atomic_store(&vigiaReiniciar, 1);
  ativo = 1; pronto = falhou = terminou = tocando = 0;
  largura = altura = durMs = 0; bufferando = 1; bufferDesde = SDL_GetTicks();
  nAudio = nLeg = 0; audioAtual = 0; legAtual = -1;
  audioNaoSup = faixasLidas = audioLogado = 0;
  comecou = 0; comecouEm = 0; audioComecou = 0; audioPend = legPend = -1;
  // A NEW SESSION RE-ARMS THE CROP. Without this the repeat belonged to the
  // FIRST video of the app's life: the window was never cleared, so every later
  // title skipped it and a crop lost early stayed lost for that whole film.
  // Resuming is what appeared to fix it, because a resume is a re-apply with
  // the plane already running.
  cropRearm();  if (!travaLeg) travaLeg = SDL_CreateMutex();
  legendaLimpar(1);
  legendaMarcarEnvio(-1);
  if (travaLeg) { SDL_LockMutex(travaLeg); legCueSessao = 0; legAbriuEm = SDL_GetTicks(); SDL_UnlockMutex(travaLeg); }
  emTrailer = 0;
  sessao++;
  velEnviada = 100;   // Player novo no host: nasce em 1x
  if (!hAbrir) { falhou = 1; printf("[video] tpk: host sem player\n"); return 0; }
  hAbrir(urlAtual, cabecalhos);
  return 1;
}

int video_tocar(const char *u) {
  snprintf(urlAtual, sizeof urlAtual, "%s", u ? u : "");
  capmkv_iniciar(urlAtual);
  mkvGeracao++; mkvN = 0; faixasNovas = 0; mkvOlhou = 0; mkvSondarJa = 0; mkvNaoMkv = 0;
  // Um fio da fonte anterior ainda na rede ve a geracao mudada e descarta.
  mkvEstado = urlAtual[0] ? 1 : 0;
  nv_recon_zerar(&recon);
  reconPermitida = reconProxima; reconProxima = 0;
  reconIniciou = 0; reconErroPend = 0; reconLinhaRede = 0;
  reconAudio = reconLeg = -1; reconFaixasPend = 0; reconBuscarMs = -1;
  return abrirSessao();
}

void video_definir_reconexao(int sim) { reconProxima = sim ? 1 : 0; }
int  video_reconectando(void) {
  return nv_recon_ativa(&recon) && (recon.pendente || !pronto) ? recon.tentativa : 0;
}

typedef struct { char url[sizeof urlAtual]; unsigned geracao; } PedidoMkv;

static void guardarMkv(const MkvFaixa *fx, int n) {
  int i;
  memcpy(mkvFx, fx, sizeof mkvFx);
  for (i = 0; i < n; i++)
    printf("[mkv] faixa num=%d tipo=%d codec=%s idioma=%s nome=%s forcada=%d canais=%d\n",
           fx[i].numero, fx[i].tipo, fx[i].codec, fx[i].idioma[0] ? fx[i].idioma : "-",
           fx[i].nome[0] ? fx[i].nome : "-", fx[i].forcado, fx[i].canais);
  fflush(stdout);
  mkvN = n;
  faixasNovas = 1;
}

static void *fioMkv(void *arg) {
  PedidoMkv *p = arg;
  MkvFaixa *fx = calloc(MKV_MAX_FAIXAS, sizeof *fx);
  int n;
  // #385: a sonda e leitura LATERAL ao video. Host que acabou de recusar
  // conexao (pre-busca da legenda, capitulos) nao recebe outra daqui.
  rede_lateral(1);
  n = fx ? mkv_faixas(p->url, fx, MKV_MAX_FAIXAS) : 0;
  if (p->geracao == mkvGeracao) {
    printf("[mkv] sonda pela rede: %d faixa(s)\n", n);
    if (n > 0) guardarMkv(fx, n);
    else mkvNaoMkv = 1;
    mkvEstado = 3;
  }
  free(fx); free(p);
  return NULL;
}

// Codec de audio do MKV que a Samsung nao toca (DTS em todas as variantes,
// TrueHD, MLP). Puro: o teste do Mac chama por video_tpk_codec_recusado.
int video_tpk_codec_recusado(const char *codecId) {
  return codecId && (!strncmp(codecId, "A_DTS", 5) || !strncmp(codecId, "A_TRUEHD", 8) ||
                     !strncmp(codecId, "A_MLP", 5));
}

// Cruza o arquivo com o que a TV listou. Roda a cada tick ate decidir, e loga
// uma vez por sessao quando ha codec recusado no arquivo.
static void avaliarAudioTv(void) {
  int i, nArq = 0, nRec = 0;
  char lista[128] = "", recusados[48] = "";
  if (audioLogado || !faixasLidas || mkvN <= 0) return;
  for (i = 0; i < mkvN; i++) {
    const char *nome;
    if (mkvFx[i].tipo != 2) continue;
    nArq++;
    nome = faixasmkv_codec(mkvFx[i].codec);
    if (!nome[0]) nome = mkvFx[i].codec;
    if (strlen(lista) + strlen(nome) + 2 < sizeof lista) {
      if (lista[0]) strcat(lista, ",");
      strcat(lista, nome);
    }
    if (video_tpk_codec_recusado(mkvFx[i].codec)) {
      nRec++;
      if (!strstr(recusados, nome) && strlen(recusados) + strlen(nome) + 2 < sizeof recusados) {
        if (recusados[0]) strcat(recusados, "/");
        strcat(recusados, nome);
      }
    }
  }
  audioLogado = 1;
  if (!nRec) return;
  printf("[audio] TV recusou %s: faixas da TV=%d, no arquivo=%d [%s]%s\n", recusados, nAudio, nArq, lista,
         nRec == nArq && nAudio == 0 ? " -> sem audio tocavel" : "");
  fflush(stdout);
  if (nRec == nArq && nAudio == 0) audioNaoSup = 1;
}

// Fio do app. Procura o cabecalho e, quando ha faixas novas, reescreve os
// rotulos. Sem mutex, como o resto deste arquivo: o rotulo sai de uma vez so.
static void sondaMkv(double pos) {
  Uint32 t = SDL_GetTicks();
  if (mkvEstado == 1 && urlAtual[0] && t - mkvOlhou >= 500) {
    unsigned char *cab = NULL; long cabN = 0;
    mkvOlhou = t;
    if (mkvass_cabecalho(urlAtual, NULL, NULL) && mkvass_cabecalho(urlAtual, &cab, &cabN)) {
      MkvFaixa *fx = calloc(MKV_MAX_FAIXAS, sizeof *fx);
      int n = fx ? mkv_faixas_do_trecho(cab, cabN, fx, MKV_MAX_FAIXAS, NULL, 0, NULL) : 0;
      printf("[mkv] sonda pelo trecho da pre-busca (%ld bytes, sem rede): %d faixa(s)\n", cabN, n);
      fflush(stdout);
      if (n > 0) { guardarMkv(fx, n); mkvEstado = 3; }
      free(fx); free(cab);
    }
    // Sem pre-busca (ou Tracks fora do trecho): Range proprio, com o video
    // andando. MP4 nao tem TrackEntry: nao vale a descida.
    if (mkvEstado == 1 && pronto && (pos >= 5.0 || mkvSondarJa)) {
      PedidoMkv *p = fonteMp4 ? NULL : malloc(sizeof *p);
      pthread_t fio;
      mkvEstado = 3;
      if (p) {
        snprintf(p->url, sizeof p->url, "%s", urlAtual);
        p->geracao = mkvGeracao;
        mkvEstado = 2;
        if (pthread_create(&fio, NULL, fioMkv, p) == 0) pthread_detach(fio);
        else { free(p); mkvEstado = 3; }
      }
    }
  }
  if (faixasNovas && mkvN > 0 && (nAudio || nLeg)) {
    int m;
    faixasNovas = 0;
    m = faixasmkv_aplicar(faixaAudio, nAudio, faixaLeg, nLeg, mkvFx, mkvN);
    printf("[mkv] %d rotulo(s) de faixa vindos do cabecalho\n", m);
    fflush(stdout);
    // O idioma pode ter chegado so agora: a preferencia de audio vale de novo.
    if (m && !reconFaixasPend) escolherAudioPreferido();
  }
  avaliarAudioTv();
}

// A deferred choice leaves once playback has really started. AUDIO leaves on the
// first tick, measured to be enough. SUBTITLE waits longer: at 0.00 s the write
// was swallowed even while Playing (the player's bookkeeping said "already on 2"
// while the demuxer kept track 0).
//
// The subtitle window used to count from EV_TOCANDO, which the host emits right
// after Start() with the buffer still filling - `tocando` does not prove a
// decoded frame exists. The 2500 ms expired during buffering and the write landed
// on nothing. Measured over five sessions on the TV, the write took effect only
// when it fell after the first frame (see the commit message for the table); the
// one session where it worked was the one whose buffer was slow.
#define LEG_ACOMODAR_MS 2500u
// The frame is proven by the position passing 0.25 s - the same signal the hole
// of #188 uses (player.c). It is the only "there is a picture" evidence Tizen
// exposes through .NET. Deliberately no ceiling: the one slow-buffer session that
// worked took 4519 ms, so a timeout would fire mid-buffer and restore the bug.
#define LEG_QUADRO_S   0.25
static int temQuadro(void) { return pronto && video_pos() >= LEG_QUADRO_S; }

static void escolhasPendentes(void) {
  Uint32 agora = SDL_GetTicks();
  // Audio keeps its own state: it leaves on the first tick and never had this
  // defect, so it must not be made to wait for the frame.
  if (!audioComecou && (tocando || temQuadro())) audioComecou = 1;
  if (audioComecou && audioPend >= 0) {
    int i = audioPend; audioPend = -1;
    printf("[video] tpk: deferred audio dispatched track=%d\n", i);
    fflush(stdout);
    if (hEscolher) hEscolher(0, faixaAudio[i].numero);
  }
  // The subtitle window counts from this, the first frame. The CROP does not: an
  // owed write is sent while the film is RUNNING (see below), which is a different
  // and much weaker condition. Keeping the two apart is the point - a paused film
  // reaches none of this and the crop must still go out.
  if (!comecou && temQuadro()) {
    comecou = 1; comecouEm = agora;
    // The position is the frame evidence and this logs it: on a RESUMED episode
    // the seek target can read >= LEG_QUADRO_S before any frame is decoded, and
    // the log would show "first frame" at 2730.12s one tick into the session.
    printf("[video] tpk: first frame at %.2fs\n", video_pos());
    fflush(stdout);
  }
  // The owed write goes out when the film is RUNNING.
  //
  // It used to go out on `cropCanLand()` (has it EVER played), which is sticky for the
  // whole session: a write dropped while paused was re-sent immediately, still paused,
  // dropped again, and never tried once the person pressed play. That is why going back
  // to Original while paused stayed zoomed even after playback started. Waiting for
  // `tocando` is what makes the resume the thing that lands it.
  if (cropOwed && tocando) cropSendOwed();
  if (!comecou) return;
  if (legPend >= 0 && agora - comecouEm >= LEG_ACOMODAR_MS) {
    int i = legPend; legPend = -1;
    printf("[video] tpk: settled subtitle dispatched track=%d (frame at %u ms)\n",
           i, (unsigned)(agora - comecouEm));
    fflush(stdout);
    legendaEnviar(i);
  }
}

static void vigiarPlayer(double pos) {
  Uint32 agora = SDL_GetTicks();
  if (atomic_exchange(&vigiaReiniciar, 0)) vigiaAtiva = 0;
  if (!ativo || !pronto || !tocando || pausaPedida || bufferando || terminou ||
      falhou || recon.pendente || reconBuscarMs >= 0) { vigiaAtiva = 0; return; }
  if (!vigiaAtiva || pos != progressoPos) {
    vigiaAtiva = 1; progressoPos = pos; progressoEm = agora; return;
  }
  if (agora - progressoEm < TPK_PRESO_MS) return;
  printf("[video] tpk preso: relogio %.3fs sem andar ha %u ms; parar e recuperar\n",
         pos, (unsigned)(agora - progressoEm));
  fflush(stdout);
  // Mesma maquina limitada da reconexao: nao cria um loop paralelo infinito.
  reconErroCod = -2; reconErroPend = 1; reconLinhaRede = 1; reconIniciou = 1;
  tocando = 0; vigiaAtiva = 0;
  if (hParar) hParar(); // host confirma/libera antes de aceitar outra abertura
}

void video_bombear(void) {
  escolhasPendentes();
  legendaVigiarSilencio(SDL_GetTicks());
  // O SetPlaybackRate exige Ready/Playing/Paused: so com o prepare feito.
  if (!velRecusada && pronto && hEscolher && velEnviada != velPedida) {
    velEnviada = velPedida;
    hEscolher(3, velEnviada);
  }
  double pos = video_pos();
  vigiarPlayer(pos);
  sondaMkv(pos);
  if (pronto && pos > 0.5) reconIniciou = 1;
  if (pronto && reconBuscarMs < 0) nv_recon_progresso(&recon, pos);
  // O seek do recarregar sai com o player ja tocando: o host da Start logo
  // depois do prepare, e um seek no meio disso concorre com ele.
  if (reconBuscarMs >= 0 && pronto && tocando) {
    if (hBuscar) hBuscar(reconBuscarMs);
    vigiaAtiva = 0;
    printf("[video] reconexao: retomado em %ds\n", reconBuscarMs / 1000);
    fflush(stdout);
    reconBuscarMs = -1;
  }
  if (reconErroPend) {
    int antes = recon.tentativa;
    int rede = nv_recon_rede_tpk(reconErroCod, NULL) || reconLinhaRede;
    reconErroPend = 0; reconLinhaRede = 0;
    if (reconPermitida && urlAtual[0] && (reconIniciou || recon.tentativa) &&
        nv_recon_erro(&recon, rede, SDL_GetTicks(), pos)) {
      if (!antes) { reconAudio = audioAtual; reconLeg = legAtual; }
      if (recon.tentativa != antes) {
        printf("[video] conexao caiu (0x%x): tentativa %d/%d, espera %us\n",
               (unsigned)reconErroCod, recon.tentativa, NV_RECON_MAX,
               nv_recon_espera_ms(recon.tentativa) / 1000u);
        fflush(stdout);
      }
      bufferando = 0;
    } else {
      if (recon.esgotou) { printf("[video] reconexao: desistiu depois de %d tentativas\n", NV_RECON_MAX); fflush(stdout); }
      falhou = 1;
    }
  }
  if (nv_recon_vencida(&recon, SDL_GetTicks())) {
    printf("[video] reconectando: alvo %.0fs, tentativa %d\n", recon.alvo, recon.tentativa);
    fflush(stdout);
    reconFaixasPend = 1;
    reconBuscarMs = recon.alvo > 1.0 ? (int)(recon.alvo * 1000.0) : -1;
    if (!abrirSessao()) { reconErroCod = -1; reconErroPend = 1; }
  }
}
void video_parar(void) {
  emTrailer = 0;
  capmkv_zerar();   // fio de capitulos em voo nao alimenta o proximo titulo
  audioPend = legPend = -1; comecou = 0; comecouEm = 0; audioComecou = 0;
  legendaLimpar(1);
  nv_recon_zerar(&recon);
  reconErroPend = 0; reconFaixasPend = 0; reconBuscarMs = -1;
  if (ativo && hParar) hParar();
  ativo = pronto = tocando = 0;
}
void video_pausar(int p) { pausaPedida = p != 0; vigiaAtiva = 0; if (hPausar) hPausar(p); }
int video_pausa_confirmada(void) { return 0; } // host nao fornece ack por sessao
void video_volume(int pct) { if (hVolume) hVolume(pct); }
void video_buscar(double s) {
  progressoEm = SDL_GetTicks(); progressoPos = video_pos(); vigiaAtiva = 1;
  if (hBuscar) hBuscar((int)(s * 1000.0));
  terminou = 0;
}
void video_janela(int x, int y, int w, int h) {
  if (hJanela) hJanela(x, y, w, h);
  // A PLAIN WINDOW WRITE SHOWS THE WHOLE SOURCE, so a crop still on the plane comes
  // off with it - with THIS destination, not a full screen. Only when one is in
  // force or owed: an untouched plane gets no write (and a crop write on a paused
  // video force-resumes it, so silence matters here).
  if (cropAtivo || cropOwed) cropClear(x, y, w, h);
}

// RECORTE DE FONTE EMULADO PELO RETANGULO DE DESTINO (ROI).
//
// O webOS recorta pela FONTE: o ACB aceita (sx,sy,sw,sh) do quadro decodificado
// mais um destino, e os modos de aspecto do player saem disso. O
// Tizen.Multimedia.Player (tizen-tpk/Video.cs) NAO tem retangulo de fonte — so
// DisplaySettings.SetRoi, que e o DESTINO na tela. A primeira versao disto
// descartava a fonte e aplicava so o destino, e o resultado era que TODO modo
// de aspecto desenhava o mesmo retangulo: na TV o botao de recorte/zoom nao
// mudava nada, em nenhum modo (#178).
//
// A conta que substitui: desenhar o recorte (sx,sy,sw,sh) dentro de
// (dx,dy,dw,dh) e o MESMO que desenhar o quadro INTEIRO num retangulo maior,
// deslocado para que o pedaco desejado caia sobre o destino.
//
//   escalaX = dw/sw            (quanto a fonte e ampliada na horizontal)
//   escalaY = dh/sh            (idem vertical)
//   W = qw * escalaX           (o quadro inteiro nessa escala)
//   H = qh * escalaY
//   X = dx - sx * escalaX      (recua a origem para o recorte cair em dx)
//   Y = dy - sy * escalaY
//
// O que sobra para fora da tela e o que o recorte descartaria. O ROI resultante
// pode ser MAIOR que a tela e ter origem NEGATIVA — e Video.cs.Janela deixa
// esse retangulo passar cru ao SetRoi (so cai em LetterBox no quadro cheio sem
// zoom).
//
// ROI FORA DA TELA NAO SAI MAIS (#188, #195). Desde este zoom (1.6.x), todo
// trailer do destaque manda um ROI assim (o zoom padrao do trailer e 1,34) e
// as S90C/S90D/QN90D (Tizen 9) mostram a tela inicial da Samsung no lugar do
// video, com o som tocando. A imagem do que esta atras do app aparecendo onde
// o furo nao tem video ja foi vista na S90D (#185). NAO PROVADO que o firmware
// apaga o plano com ROI fora do painel, mas e o que o webOS faz (player.c,
// aplicarAspecto: "retangulo fora do painel nao e recorte, e retangulo
// invalido: o plano apaga") e o zoom nunca foi visto funcionando numa Samsung.
// Por isso o padrao volta a ser o de antes do zoom: video_recorte_fonte() = 0
// (o player fica nos modos sem recorte, o trailer sem zoom) e, se alguem
// chamar isto mesmo assim, um ROI que sai da tela vira o destino cru.
// NV_TPK_ZOOM_ROI=1 liga o zoom de novo, so para canario.
#ifndef NV_TPK_ZOOM_ROI
#define NV_TPK_ZOOM_ROI 0
#endif
// #203 NO LONGER GATES THIS ON A BUILD FLAG. It used to compile the crop/zoom
// modes on for the 4/5 package on the theory that moving the video window was
// enough. It is not: a zoom needs the frame drawn larger than the screen, with its
// origin off the panel, and this platform refuses a negative origin (see the note
// on cropSource). Whether the modes may be offered is now the TV's own answer - the
// host probes for the source crop and reports through nv_tpk_video_crop_available
// (see video_recorte_fonte).
// Flag de EXECUCAO (#241, #290): Ajustes > Trailers > "Zoom no trailer e no
// player (experimental)" deixa cada dono testar na propria TV. Comeca no padrao de compilacao acima.
static int zoomRoi = 0;
// 1 so enquanto o que toca e um TRAILER (trailer.c marca depois do video_tocar);
// zerado a cada abertura e em video_parar. So informativo desde o #290: com o
// ajuste ligado o player tambem ganha ROI fora da tela.
void video_tpk_trailer_marcar(int sim) { emTrailer = sim ? 1 : 0; }
void video_tpk_zoom_roi_definir(int ligado) {
  ligado = ligado ? 1 : 0;
  if (ligado == zoomRoi) return;
  zoomRoi = ligado;
  printf("[trailer] tpk zoom ROI (trailer + player): %s (setting)\n", ligado ? "on" : "off");
  fflush(stdout);
}
int video_tpk_zoom_roi(void) { return zoomRoi; }
#define TPK_TELA_W 1920   // o host (Program.cs) usa 1920x1080 e o layout tambem
#define TPK_TELA_H 1080
static int ultRoiX, ultRoiY, ultRoiW, ultRoiH, temRoi;
static unsigned roiForaLogado;
static int roiNaTela(int x, int y, int w, int h) {
  return w > 0 && h > 0 && x >= 0 && y >= 0 && x + w <= TPK_TELA_W && y + h <= TPK_TELA_H;
}

// A CROP OF THE SOURCE, IN RATIOS OF THE FRAME.
//
// This is the Samsung pair of the webOS sourceInput/displayOutput, and it is the
// only way to zoom here without moving the destination off the screen. Ratios and
// not pixels because that is what the call takes, and because a ratio can never be
// negative: a zoom always needs an origin left of and above the screen, and this
// platform refuses a negative origin. MEASURED: `mmf_attribute_validate_int >
// [mmf_attribute:display_win_roi_x] out of range` (TizenFX #514), and Samsung's
// own answer there is "libmm_player don't accept negative position when rendering
// by Overlay" - which is the type of display this host uses.
//
// The destination is the WHOLE screen, so nothing ever leaves it. On success the
// plane is already correct and the caller must not also move it.
//
static int cropSource(int qw, int qh, int sx, int sy, int sw, int sh,
                        int dx, int dy, int dw, int dh) {
  if (!hCropSource || qw < 2 || qh < 2 || sw < 2 || sh < 2) return 0;
  // Ratios of the decoded frame. The slice is already clamped to the frame and
  // even-aligned by player.c, so it never exceeds 0..1.
  { double rx = (double)sx / (double)qw, ry = (double)sy / (double)qh;
    double rw = (double)sw / (double)qw, rh = (double)sh / (double)qh;
    int r;
    if (rx < 0.0) { rw += rx; rx = 0.0; }
    if (ry < 0.0) { rh += ry; ry = 0.0; }
    if (rw > 1.0) rw = 1.0;
    if (rh > 1.0) rh = 1.0;
    if (rw <= 0.0 || rh <= 0.0) return 0;
    r = hCropSource(rx, ry, rw, rh, dx, dy, dw, dh);
    // SAY WHAT THIS NUMBER IS. The host has to marshal the call to its own thread,
    // so what comes back is the PROBE verdict - "this TV has the call" - not the
    // result of these values. Printing it as "applied" would be a claim the caller
    // cannot support.
    printf("[video] tpk source crop %.3f,%.3f %.3fx%.3f -> sent (host says; the verdict is its own, not this call's)\n",
           rx, ry, rw, rh);
    fflush(stdout);
    // UNDO THE FORCED RESUME. SetVideoRoi resumes playback by force - the vendor
    // says so in the note for the call - so a crop asked for on a video the person
    // PAUSED starts the film, and the app then sees it as "the user pressed play".
    // There is nothing to negotiate: the app knows it was paused, so it says so
    // again right after, and the pause wins. The two calls are queued in order.
    if (r && pausaPedida) {
      printf("[video] tpk source crop: re-asserting the pause it just resumed\n");
      fflush(stdout);
      if (hPausar) hPausar(1);
    }
    if (r) { temRoi = 0; cropAtivo = 1; return 1; }
  }
  return 0;
}

// Back to the untouched frame. Sent when the mode stops cropping - and from
// video_janela whenever a plain window write supersedes one, which is what keeps a
// zoom off the PiP corner and out of every later mode.
//
// THE DESTINATION GOES WITH IT. The release writes the fit back (Video.cs reads the
// crop's own destination for it), and it must be the fit the CALLER just asked for:
// a hardcoded full screen clobbered the credits shrink and the corner window, which
// the window write before this call had just set.
static void cropClear(int dx, int dy, int dw, int dh) {
  // A ZOOM ASKED FOR BEFORE THE FILM PLAYED MUST NOT OUTLIVE ITS OWN DEMAND. This
  // path does not go through cropSource, so it has to drop the held zoom itself or
  // the next retry would put the old crop back - and it drops it BEFORE the send:
  // a clear that fails must still kill the held zoom, it may only retry the write.
  cropOwed = 0; owedIdentity = 0;
  if (!hCropSource) { cropAtivo = 0; return; }
  if (hCropSource(0.0, 0.0, 1.0, 1.0, dx, dy, dw, dh)) {
    // THE WHOLE FRAME IS ALSO OWED WHILE THE FILM IS PAUSED.
    //
    // MEASURED: going back to Original while paused left the picture zoomed, and it
    // stayed zoomed after playback resumed. The zoom path had a retry for exactly
    // this (a write the plane drops while paused) and this path had none: it was a
    // one-shot. Cycling through every mode appeared to fix it only because the last
    // round left the plane somewhere the stale crop no longer matched.
    //
    // The retry is armed by the same rule as the crop: it is armed when the film is
    // not running now, and sent by cropSendOwed() once it is.
    if (!tocando) { cropOwed = 1; owedIdentity = 1;
      owedDstX = dx; owedDstY = dy; owedDstW = dw; owedDstH = dh; }
    cropAtivo = 0;
    printf("[video] tpk source crop cleared%s\n", cropOwed ? " (owed; the film is paused)" : "");
    fflush(stdout);
  } else {
    // The write did not leave this side: the crop may still be on the plane, so the
    // next plain write has to try again.
    cropAtivo = 1;
  }
}


void video_janela_fonte(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
  double qw = largura, qh = altura, ex, ey;
  int X, Y, W, H;

  // Sem as dimensoes do quadro, ou sem recorte de verdade, o destino cru serve.
  if (qw < 2.0 || qh < 2.0 || sw <= 0 || sh <= 0) { temRoi = 0; video_janela(dx, dy, dw, dh); return; }
  // Recorte que cobre o quadro inteiro E o caso sem zoom: mesma coisa. The crop
  // comes off with it: video_janela sends the whole frame whenever one is in force.
  if (sx <= 0 && sy <= 0 && sw >= (int)qw && sh >= (int)qh) {
    temRoi = 0; video_janela(dx, dy, dw, dh);
    return;
  }

  printf("[video] tpk recorte %d,%d %dx%d de %.0fx%.0f\n", sx, sy, sw, sh, qw, qh);
  fflush(stdout);

  // THE CROP FIRST: it is the only path that needs no rectangle outside the
  // screen, and it is what actually zooms. The destination trick below needs the
  // whole frame drawn into a LARGER rectangle whose origin sits left of and above
  // the screen, and this platform refuses a negative origin - see the note on
  // cropSource. It is kept only as the last resort.
  //
  // 4/5 ONLY, for now. The host is shared by all four packages and this path is
  // verified on one TV; enabling the crop on the 6+/9 builds would change the
  // aspect modes there with nothing to test it against. They keep today's
  // behaviour until someone can look at one.
#ifdef NV_TPK40
  { int r = cropSource((int)qw, (int)qh, sx, sy, sw, sh, dx, dy, dw, dh);
    if (r) {
      // Remember the slice. A write made while the film is paused may be dropped by
      // the plane, so it is marked owed and cropSendOwed() sends it once the film is
      // running (see escolhasPendentes).
      owedSrcX = sx; owedSrcY = sy; owedSrcW = sw; owedSrcH = sh;
      owedDstX = dx; owedDstY = dy; owedDstW = dw; owedDstH = dh;
      // ARMED WHENEVER THE FILM IS NOT RUNNING, not merely before the first play. The
      // old rule was "has it EVER played", which is sticky for the whole session, so a
      // crop asked for while paused was never treated as owed.
      cropOwed = tocando ? 0 : 1;
      owedIdentity = 0;
      return;
    } }
#endif

  ex = (double)dw / (double)sw;
  ey = (double)dh / (double)sh;
  W  = (int)(qw * ex + 0.5);
  H  = (int)(qh * ey + 0.5);
  X  = (int)(dx - sx * ex + 0.5);
  Y  = (int)(dy - sy * ey + 0.5);

  // #290: o ajuste opt-in vale para o trailer E para o player (proporcao).
  if (!NV_TPK_ZOOM_ROI && !zoomRoi && !roiNaTela(X, Y, W, H)) {
    if (roiForaLogado != sessao) {
      roiForaLogado = sessao;
      printf("[video] tpk no zoom: the crop refused and the ROI would leave the screen\n");
      fflush(stdout);
    }
    temRoi = 0;
    video_janela(dx, dy, dw, dh);
    return;
  }

  printf("[video] tpk emulated crop -> roi %d,%d %dx%d\n", X, Y, W, H);
  fflush(stdout);
  ultRoiX = X; ultRoiY = Y; ultRoiW = W; ultRoiH = H; temRoi = 1;
  video_janela(X, Y, W, H);
}
// The zoom needs the source crop. Without it, five of the eight modes draw the
// same full screen on a 16:9 frame and the other three would do nothing - a
// button that lies. The host probes and reports; see nv_tpk_video_crop_available.
// 4/5 only: the 6+/9 builds keep today's modes (see video_janela_fonte).
int  video_recorte_fonte(void) {
#ifdef NV_TPK40
  return cropOk || NV_TPK_ZOOM_ROI || zoomRoi;
#else
  return NV_TPK_ZOOM_ROI || zoomRoi;
#endif
}
int  video_recorte_fonte_trailer(void) { return NV_TPK_ZOOM_ROI || zoomRoi; }
// O host prende o plano em mais de um ponto depois do prepare; um ROI pedido
// cedo pode ser engolido. trailer.c/player.c repetem o pedido nos primeiros
// segundos por aqui — reenvia o ultimo ROI calculado, sem recalcular.
void video_recorte_reaplicar(void) { if (temRoi) video_janela(ultRoiX, ultRoiY, ultRoiW, ultRoiH); }
const char *video_url_atual(void) { return urlAtual; }
double video_pos(void) { return (hPos && pronto) ? hPos() / 1000.0 : 0; }
double video_duracao(void) { return durMs / 1000.0; }
// Capitulos do MKV lidos por um fio lateral (capmkv.c, 203-capitulos).
double video_creditos(void) { return capmkv_creditos(video_duracao()); }
double video_buffer_fim(void) { return 0; }
unsigned video_bufferando_ms(void) {
  // Esperando para reconectar: o watchdog (app.c) nao troca de fonte.
  if (nv_recon_ativa(&recon)) return 0;
  return bufferando ? SDL_GetTicks() - bufferDesde : 0;
}
void video_definir_dv(int dv) { (void)dv; }
void video_definir_cabecalhos(const char *c) { snprintf(cabecalhos, sizeof cabecalhos, "%s", c ? c : ""); }
void video_definir_mp4(int m) { fonteMp4 = m != 0; }
int  video_tocando(void) { return tocando; }
int  video_pronto(void) { return pronto; }
int  video_ativo(void) { return ativo; }
int  video_falhou(void) { return falhou; }
int  video_audio_nao_suportado(void) { return audioNaoSup; }
int  video_seek_desistiu(void) { return 0; }
int  video_terminou(void) { return terminou; }
int  video_conflito_recurso(void) { return conflito; }

// OUTRO APP COM O VIDEO DA TV (#178, rawldon AU7000 e mais 2 TVs nos registros
// 10257-10419). O host avisa a interrupcao so como EV_PAUSADO; aqui isso so
// zerava `tocando`, `pronto` seguia 1 e o trailer continuava "tocando" com o
// furo aberto — e o que aparecia no furo era o video do YouTube, dono do plano.
// A razao so chega pela linha de log do host (Video.cs: "interrompido: " +
// Reason); o clipe mudo do arranque loga "prime interrompido ..." e fica de
// fora de proposito. Conflito = a fonte falhou: o trailer fecha e volta a
// arte, o player do filme cai no caminho de erro de sempre.
// So conta com o Nuvio NA FRENTE. Nos logs da 1.6.0 metade dos ResourceConflict
// vem logo depois de "[janela] principal visivel=False": a pessoa saiu do app
// no meio do filme e a TV tomou o video, o que e so uma pausa. E o player
// PRINCIPAL nunca e marcado como falho aqui — isso faria o automatico trocar de
// fonte ao voltar; quem le `conflito` e so o trailer.
static volatile int janelaVisivel = 1;
void video_tpk_log_host(const char *linha) {
  if (!linha) return;
  // "erro ConnectionFailed" vem logo antes do evento 5 (Video.cs).
  if (!strncmp(linha, "erro ", 5) && nv_recon_rede_tpk(0, linha)) reconLinhaRede = 1;
  // O player acusa o codec sem lista vazia: mesmo aviso, com a linha de prova.
  if (!strncmp(linha, "erro NotSupportedAudioCodec", 27)) {
    printf("[audio] TV recusou o audio da fonte: player acusou NotSupportedAudioCodec (faixas da TV=%d)\n", nAudio);
    fflush(stdout);
    audioNaoSup = 1;
  }
  if (strstr(linha, "[janela] principal visivel=")) {
    janelaVisivel = strstr(linha, "visivel=True") != NULL;
    return;
  }
  if (!strstr(linha, "interrompido: ResourceConflict") || !janelaVisivel) return;
  tocando = 0;
  if (!conflito) { printf("[video] tpk: outro app tomou o video da TV; trailers automaticos desligados nesta sessao\n"); fflush(stdout); }
  conflito = 1;
}
int  video_n_audio(void) { return nAudio; }
int  video_n_legenda(void) { return nLeg; }
const VideoFaixa *video_audio(int i) { return (i >= 0 && i < nAudio) ? &faixaAudio[i] : 0; }
const VideoFaixa *video_legenda(int i) { return (i >= 0 && i < nLeg) ? &faixaLeg[i] : 0; }
int  video_legenda_ordinal_mkv(int i) { return (i >= 0 && i < nLeg) ? faixaLeg[i].ordinalMkv : -1; }
// #269: a sonda de verdade (antes era sempre 2, "nao e MKV", e nenhuma faixa
// ia ao overlay). 0 enquanto procura, ou com o cabecalho lido e ainda nao
// aplicado as faixas (sondaMkv, no proximo bombear); 2 quando nao e MKV.
int  video_mkv_sondado(void) {
  if (!urlAtual[0] || fonteMp4 || mkvNaoMkv) return 2;
  if (mkvEstado == 1 || mkvEstado == 2) return 0;
  if (faixasNovas && mkvN > 0 && (nAudio || nLeg)) return 0;
  return 1;
}
// Barato: a folha chama a cada quadro enquanto espera. O Range sai no proximo
// bombear, sem esperar os 5 s de video (precisa do prepare feito).
void video_sondar_mkv_agora(void) { if (mkvEstado == 1) mkvSondarJa = 1; }
int  video_audio_atual(void) { return audioAtual; }
int  video_legenda_atual(void) { return legAtual; }
void video_escolher_audio(int i) {
  if (i < 0 || i >= nAudio) return;
  audioAtual = i;
  // Deferred until the first tick: a write before Start is accepted and ignored.
  // The tick, not the frame - audio was measured working that way.
  if (!audioComecou) { audioPend = i; return; }
  if (hEscolher) hEscolher(0, faixaAudio[i].numero);
}
void video_escolher_legenda(int i) {
  if (i >= nLeg) return;
  legAtual = i;
  // Every new choice supersedes an older deferred one, including immediate
  // choices made after the settle window but before the next pump.
  legPend = -1;
  legendaLimpar(1);
  if (i < 0) legendaMarcarEnvio(-1);
  if (i >= 0 && hEscolher) {
    if (!comecou || SDL_GetTicks() - comecouEm < LEG_ACOMODAR_MS) { legPend = i; return; }
    legendaEnviar(i);
  }
}

int  video_legenda_nativa(char *d, int t) {
  if (!d || t < 2) return 0;
  d[0] = 0;
  if (!ativo || legAtual < 0 || !travaLeg) return 0;
  SDL_LockMutex(travaLeg);
  if (!legCuesBloqueados && legTexto[0] && (Sint32)(legAte - SDL_GetTicks()) > 0) snprintf(d, (size_t)t, "%s", legTexto);
  SDL_UnlockMutex(travaLeg);
  return d[0] != 0;
}
void video_legenda_externa(const char *u) { (void)u; }
// O atraso do estilo vale para a embutida (o player desloca o evento).
void video_legenda_estilo(const VideoLegendaEstilo *e) { if (e && hEscolher) hEscolher(2, e->atrasoMs); }
int  video_tem_atmos(void) { return 0; }
int  video_tem_dolby_vision(void) { return 0; }
const char *video_hdr(void) { return "none"; }
// Numeric host error is useful evidence even when no textual cause is exposed.
// Formatting happens on the app thread; the callback only publishes atomics.
const char *video_erro_texto(void) {
  static char texto[64];
  if (!atomic_load(&temErroDetalhe)) return "";
  snprintf(texto, sizeof texto, "Samsung player error 0x%08x",
           atomic_load(&erroDetalhe));
  return texto;
}
int video_decoder_anunciou(void) { return 1; }
// Escala do alvo de desenho (GPU adaptativa, LG): o host .tpk nao usa.
void video_escala_definir(int sw, int sh) { (void)sw; (void)sh; }
int  video_largura(void) { return largura; }
int  video_altura(void) { return altura; }
int  video_pode_forcar_sdr(void) { return 0; }
int  video_velocidade_suportada(void) { return !velRecusada && hEscolher != NULL; }
void video_velocidade(int c) {
  if (c <= 0 || c > 400) c = 100;
  if (!velRecusada) velPedida = c;
}
int  video_velocidade_atual(void) { return velRecusada ? 100 : velPedida; }
void video_velocidade_recusada(void) {
  velRecusada = 1; velPedida = 100;
  if (ativo && pronto && hEscolher && velEnviada != 100) hEscolher(3, 100);
  velEnviada = 100;
}
int  video_velocidade_bloqueada(void) { return 0; }
void video_forcar_sdr(void) {}
void video_encerrar(void) { video_parar(); }
#endif
