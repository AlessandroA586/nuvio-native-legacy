/* DTS / TrueHD -> Dolby Digital on the Samsung .tpk: the local HTTP server the
 * TV's own player reads the converted stream from (docs/plans/
 * dts-truehd-ac3-tizen-DESIGN.md, "Option B"). The converter is dts_remux.c.
 *
 * Delivery is a TEST SETTING for now (Settings > Playback > More options, .tpk
 * only): the S95C decides which one stays. Values are the persisted indexes
 * of ajustes.c (V_DTS_TPK_MODO / V_DTS_TPK_CORPO): append only.
 *
 * Every function is safe from the app thread: nothing here blocks on the
 * network. Only the .tpk compiles the server; elsewhere these are stubs. */
#ifndef NV_DTS_HLS_TPK_H
#define NV_DTS_HLS_TPK_H
#include <stddef.h>

enum {
  DTSTPK_HLS = 0,      /* 1: HLS VOD, segments made on request, 127.0.0.1 */
  DTSTPK_PROG_TS,      /* 2: one progressive MPEG-TS, restarted on seek   */
  DTSTPK_PROG_MKV,     /* 3: one progressive Matroska, restarted on seek  */
  DTSTPK_PROG_MP4,     /* 3: one progressive fragmented MP4               */
  DTSTPK_HLS_LAN,      /* 4: HLS like 1, served on the TV's LAN address   */
  DTSTPK_DESLIGADO,    /* 5: off, the app behaves as before (control)     */
  DTSTPK_MODOS
};
enum { DTSTPK_CORPO_CHUNKED = 0, DTSTPK_CORPO_LENGTH, DTSTPK_CORPO_CLOSE, DTSTPK_CORPOS };
enum { DTSTPK_NADA = 0, DTSTPK_SONDANDO = 1, DTSTPK_PRONTO = 2, DTSTPK_FALHOU = -1 };

/* 1 when this build can convert (FFmpeg linked, decoders and muxer present). */
int dts_tpk_disponivel(void);
const char *dts_tpk_modo_nome(int modo);
const char *dts_tpk_corpo_nome(int corpo);

/* New session for `url` (raw "Name: value" header lines): converts the k-th
 * audio track of the file, counted from 0 in container order (-1 = the first
 * DTS/TrueHD/MLP). The source is
 * probed in a thread; the URL written to `out` is what the player opens (its
 * requests wait for the probe). inicio: start of the first progressive URL.
 * 0 when the server cannot run. Replaces the previous session. */
int dts_tpk_abrir(const char *url, const char *headers, int audio_ordinal, int stereo_aac,
                  int modo, int corpo, double inicio, char *out, size_t n);
/* Progressive modes: the URL that restarts the stream at s (same session). */
int dts_tpk_url_em(double s, char *out, size_t n);
/* Ends the session: cancels the probe and every generator, logs a summary. */
void dts_tpk_fechar(void);
int  dts_tpk_estado(void);
const char *dts_tpk_erro(void);          /* reason of DTSTPK_FALHOU, no URL */
double dts_tpk_duracao(void);            /* seconds, once ready */
int  dts_tpk_modo(void);                 /* mode of the session, -1 none */
/* Progressive: source time of the keyframe the CURRENT stream starts at (the
 * player counts from there). -1 until the stream's request has started. */
double dts_tpk_inicio_stream(void);
const char *dts_tpk_caminho(void);       /* decoder path, e.g. "DTS core only" */
/* 1 when the session FAILED because HLS needs a keyframe index the source
 * lacks (MKV without Cues): a progressive mode can still play it. */
int  dts_tpk_sem_indice(void);
int  dts_tpk_e_url(const char *url);     /* is this a URL of this server */

/* LAN log endpoint (no `sdb shell` on retail TVs): started on first call;
 * writes http://<TV-IP>:<port>/<token>/ into url. 0 without a LAN address. */
int dts_tpk_registro_rede(char *url, size_t n);
#endif
