// #421: a escolha real mantem a TV durante a colheita; ASS nao muda.
#include "../src/faixas.c"
#include <assert.h>
static VideoFaixa faixa;
static int nativa, ordinal;
const VideoFaixa *video_legenda(int i) { assert(i == 2); return &faixa; }
void video_escolher_legenda(int i) { nativa = i; }
const char *video_url_atual(void) { return "https://example.test/texto.mkv"; }
void mkvass_iniciar_ordinal(const char *url, int ord) { assert(*url); ordinal = ord; }
int main(void) {
  strcpy(faixa.codec, "S_TEXT/UTF8");
#ifdef NV_ANDROID
  assert(!vaiOverlay(&faixa)); // Media3 ja fornece o texto.
#else
  assert(vaiOverlay(&faixa));
  overlayAssumir(2, 7);
  assert(nativa == 2 && ordinal == 7 && legOverlayTV && legOverlay == 2);
  assert(legOverlayFalhas == 0 && legOverlayRetomar == 0);
  strcpy(faixa.codec, "S_TEXT/WEBVTT"); assert(vaiOverlay(&faixa));
#endif
  strcpy(faixa.codec, "S_TEXT/ASS");
  assert(vaiOverlay(&faixa)); overlayAssumir(2, 7);
  assert(nativa == -1 && !legOverlayTV);
  strcpy(faixa.codec, "S_HDMV/PGS"); assert(!vaiOverlay(&faixa));
  puts("faixas_texto: escolha e fallback inicial LG/TPK/Android ok");
}
