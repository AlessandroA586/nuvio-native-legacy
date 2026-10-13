// #486: addon so de torrent (Torrentio sem debrid), P2P desligado. Os torrents
// eram descartados calados: a folha so falava do debrid e o cartao do player
// era o generico. Com P2P neste pacote, a lista tem de dizer quantos o P2P
// tocaria; ligado (ou com debrid), nada a sugerir.
#include <assert.h>
#include "../src/streams.c"

static int p2pLig, motor = 1, debrid;
int debrid_resolver_cache(const char *h, int f, char *u, unsigned t, int *fora) {
  (void)h; (void)f; (void)u; (void)t; (void)fora; return 0;
}
int rede_url_final_tipo(const char *u, int s, const char *const *c, char *d,
                        unsigned t, int *st, char *m, unsigned mt, long *b) {
  (void)u; (void)s; (void)c; (void)d; (void)t; (void)st; (void)m; (void)mt; (void)b; return 0;
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)u; (void)s; (void)c; (void)st; return NULL;
}
int ajustes_selos_coloridos(void) { return 0; }
void selospacote_colorido(int ligado) { (void)ligado; }
int selospacote_embutidos_ok(void) { return 0; }
const SeloFiltro *selospacote_filtro(unsigned short id) { (void)id; return NULL; }
const char *i18n(const char *s) { return s; }
int vazao_url_aviso(const char *url) { (void)url; return 0; }
int ajustes_fonte_ordem_addon(void) { return 0; }
int ajustes_fonte_texto_addon(void) { return 0; }
int debrid_ativo(void) { return debrid; }
int p2p_ativo(void) { return p2pLig; }
int p2pmotor_disponivel(void) { return motor; }
const char *ajustes_p2p_url(void) { return ""; }
int selospacote_ativo(void) { return -1; }
unsigned selospacote_versao(void) { return 1; }
int selospacote_casar(const char *const *p, int q, unsigned short *o, int m) {
  (void)p; (void)q; (void)o; (void)m; return 0;
}
int ondever_n(const char *id) { (void)id; return 0; }
Uint32 SDL_GetTicks(void) { return 1000; }
char *dados_ler(const char *nome) { (void)nome; return NULL; }
int dados_gravar(const char *nome, const char *s) { (void)nome; (void)s; return 1; }

static const char *TORRENTIO = "{\"streams\":["
  "{\"name\":\"Torrentio\\n2160p\",\"title\":\"Movie.2160p.WEB-DL\",\"infoHash\":\"0123456789abcdef0123456789abcdef01234567\",\"fileIdx\":0},"
  "{\"name\":\"Torrentio\\n1080p\",\"title\":\"Movie.1080p.WEB-DL\",\"infoHash\":\"1123456789abcdef0123456789abcdef01234567\",\"fileIdx\":1},"
  "{\"name\":\"Torrentio\\n720p\",\"title\":\"Movie.720p.WEB-DL\",\"infoHash\":\"2123456789abcdef0123456789abcdef01234567\"}]}";

static void chegar(void) {
  Stream *a = NULL;
  assert(stream_extrair(TORRENTIO, "Torrentio", &a) == 3);
  stream_definir_lista(NULL, 0);
  stream_lista_acrescentar(a, 3, 0);
  free(a);
}

int main(void) {
  chegar();
  assert(n == 0);                                   // descartados: nada toca
  if (stream_torrents_para_p2p() != 3) {
    fprintf(stderr, "FAIL: P2P desligado, 3 torrents descartados, dica=%d\n", stream_torrents_para_p2p());
    return 1;
  }
  motor = 0; assert(stream_torrents_para_p2p() == 0); motor = 1;   // .wgt: sem P2P
  debrid = 1; assert(stream_torrents_para_p2p() == 0); debrid = 0;
  p2pLig = 1; chegar();
  assert(n == 3 && stream_qtd_torrents() == 3 && stream_torrents_para_p2p() == 0);
  puts("p2p_dica486: PASS (P2P desligado aponta o ajuste; ligado, sem motor ou com debrid, nada)");
  return 0;
}
