// SECOND SUBTITLE SESSION (F04, 1.8): an EXTERNAL SRT/VTT file shown at the
// same time as the primary subtitle, as its own immutable LegendaDocumento.
//
// It is deliberately independent of the primary overlay. Nothing here calls
// legenda_carregar, assrender_carregar or mkvass_iniciar*: those own the ONE
// primary overlay (legenda.c singleton, the single libass renderer, the single
// MKV collector) and calling them for a secondary would silently replace the
// primary. What the secondary does have:
//   - one worker thread, bounded download (LEG2_MAX_BYTES, LEG2_PRAZO_MS),
//     cancellation checked during the transfer;
//   - two generation guards: the SESSION (media; legenda2_reiniciar) and the
//     SELECTION (every choose/turn-off). A late download is published only if
//     both still match, so a file for the previous episode/choice never shows;
//   - its own offset (manual + optional automatic from AutoSync), applied
//     exactly once in legenda2_cues, and AutoSync's time map for framerate /
//     cut corrections (legenda2_definir_tempo);
//   - explicit states: loading, active, failed, unsupported. A selection is
//     never reported ACTIVE before a usable document was published.
//
// Unsupported, by design and stated in the UI: embedded tracks and ASS/SSA
// as the SECOND subtitle (no independent extractor / no second libass
// context). An ASS file is detected after download and refused without
// touching the primary.
//
// Thread model: every function except the worker is called from the UI/draw
// thread. The worker only downloads/parses and publishes under the mutex.
#ifndef NV_LEGENDA2_H
#define NV_LEGENDA2_H
#include "legenda.h"
#include <stdint.h>

#define LEG2_MAX_BYTES (4L * 1024L * 1024L)
#define LEG2_PRAZO_MS  20000u

typedef enum {
  LEG2_NENHUMA = 0,     // no second subtitle
  LEG2_CARREGANDO,      // download/parse in progress for the current selection
  LEG2_ATIVA,           // a usable document is published and drawn
  LEG2_FALHOU,          // network/limit/empty file: nothing drawn
  LEG2_NAO_SUPORTADA    // format not supported as second subtitle (ASS/SSA)
} Leg2Estado;

// Downloader used by the worker. Must honour `parar` (non-zero = abandon) and
// return 0 with a malloc'd body in *corpo/*n, or non-zero on failure. Tests
// replace it; the default uses rede_pedir with the byte/time caps above.
typedef int (*Leg2Baixador)(const char *url, long maxBytes, unsigned prazoMs,
                            int (*parar)(void *), void *u, char **corpo, long *n);
void legenda2_definir_baixador(Leg2Baixador b);   // NULL = default

// New media session: cancels any download, drops the document, offset back to
// zero. Called when faixas resets (new playback).
void legenda2_reiniciar(void);
uint64_t legenda2_sessao(void);

// Choose an external file as the second subtitle. `identidade` is the opaque
// candidate id (never a URL); `url` is used privately to download and is
// never logged. Returns the new selection generation.
unsigned legenda2_escolher(const char *identidade, const char *url,
                           const char *idioma, const char *origem);
// Turn the second subtitle off (also cancels a download in flight).
void legenda2_desligar(void);

Leg2Estado legenda2_estado(void);
// Identity of the current selection ("" = none). Selection is not ACTIVE
// until legenda2_estado() says so.
const char *legenda2_identidade(void);

// Offsets in ms, positive = earlier (same sign as legenda_cues).
int  legenda2_offset_manual(void);
void legenda2_definir_offset_manual(int ms);
void legenda2_definir_offset_auto(int ms);   // AutoSync (F05); 0 = none
int  legenda2_offset_total(void);

// Cues of the second document at `posSeg`, with the total offset applied
// once. 0 when nothing is active.
int legenda2_cues(double posSeg, LegendaCue *dst, int max);
// AutoSync (2.1): video time -> subtitle time for an accepted scale/pieces
// result (legsync.c). Called by legenda2_cues OUTSIDE this module's lock, so
// it may take its own. NULL = identity.
void legenda2_definir_tempo(double (*tempo)(double posSeg));
// Retained reference to the published document (NULL = none). Caller must
// legenda_documento_liberar. For AutoSync (F05).
LegendaDocumento *legenda2_documento(void);

// Stops and joins the worker (teardown/tests). Safe to call twice.
void legenda2_encerrar(void);

#endif
