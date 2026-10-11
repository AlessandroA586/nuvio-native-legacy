// Identidade de VOD, validade no disco e proxima fonte na ordem do addon.
#include "fontesmortas.h"
#include "streams.h"
#include "selospacote.h"
#include "fonteregra.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static time_t agora = 1000000;
static char *disco;
time_t time(time_t *t) { if (t) *t = agora; return agora; }
char *dados_ler(const char *nome) {
  assert(!strcmp(nome, "fontesmortas.txt"));
  return disco ? strdup(disco) : NULL;
}
int dados_gravar(const char *nome, const char *s) {
  assert(!strcmp(nome, "fontesmortas.txt"));
  free(disco); disco = strdup(s); return disco != NULL;
}
Uint32 SDL_GetTicks(void) { return 1000; }
int debrid_ativo(void) { return 0; }
int p2p_ativo(void) { return 0; }
int selospacote_ativo(void) { return -1; }
unsigned selospacote_versao(void) { return 1; }
int selospacote_casar(const char *const *p, int n, unsigned short *o, int m) {
  (void)p; (void)n; (void)o; (void)m; return 0;
}
void selospacote_colorido(int c) { (void)c; }
int selospacote_embutidos_ok(void) { return 0; }
const SeloFiltro *selospacote_filtro(unsigned short id) { (void)id; return NULL; }
int ajustes_selos_coloridos(void) { return 0; }
int ajustes_fonte_escopo(void) { return 0; }
int ajustes_fonte_regex_modo(void) { return 0; }
int ajustes_fonte_usar_outros(void) { return 0; }
unsigned fonteregra_versao(void) { return 0; }
int fonteregra_grupo(const FonteRegraCfg *c, const char *nome, int plugin, const char *texto) {
  (void)c; (void)nome; (void)plugin; (void)texto; return 0;
}
uint64_t badges_bit(const char *s) { (void)s; return 0; }
uint64_t badges_detectar(const char *s) { (void)s; return 0; }
int naovideo_nome(const char *a, const char *b, int h, long t) {
  (void)a; (void)b; (void)h; (void)t; return 0;
}

int main(void) {
  Stream *s = calloc(4, sizeof *s);
  assert(s);
  strcpy(s[0].provedor, "Addon A");
  strcpy(s[0].url, "https://CDN.example/movie/1.mp4?token=abc&id=7&expires=123");
  s[0].fileIdx = -1;
  s[1] = s[0];
  strcpy(s[1].url, "https://cdn.EXAMPLE/movie/1.mp4?id=7&expires=999&token=xyz");
  s[2] = s[0]; strcpy(s[2].provedor, "Addon B");
  s[3] = s[0]; strcpy(s[3].url, "https://cdn.example/movie/1.mp4?id=8");
  fontesmortas_iniciar();
  fontesmortas_marcar(&s[0]);
  assert(fontesmortas_idade(&s[1]) == 0);
  assert(fontesmortas_idade(&s[2]) < 0 && fontesmortas_idade(&s[3]) < 0);
  assert(disco && !strstr(disco, "token") && !strstr(disco, "movie"));
  // Releitura de disco e poda: fonte continua marcada depois do reinicio.
  fontesmortas_iniciar();
  agora += 60;
  assert(fontesmortas_idade(&s[1]) == 60);
  // TV arrancando com o relogio atrasado nao apaga a tabela (fontepref.c).
  agora -= 3600;
  fontesmortas_iniciar();
  assert(fontesmortas_idade(&s[1]) >= 0);
  agora += 3600;
  assert(fontesmortas_idade(&s[1]) == 60);
  // Torrent resolvido ganha URL nova, mas a identidade e hash + arquivo.
  strcpy(s[0].infoHash, "AABBCCDD"); s[0].fileIdx = 2;
  s[1] = s[0]; strcpy(s[1].url, "https://outro.invalid/novo.mp4?sig=novo");
  fontesmortas_marcar(&s[0]);
  assert(fontesmortas_idade(&s[1]) == 0);
  s[1].fileIdx = 3; assert(fontesmortas_idade(&s[1]) < 0);
  agora += 6 * 3600;
  assert(fontesmortas_idade(&s[0]) < 0);
  fontesmortas_iniciar();
  assert(disco && !strstr(disco, "AABBCCDD"));
  // Ordem de exibicao difere dos indices de chegada: A(2), B(0), C(1).
  memset(s, 0, 4 * sizeof *s);
  for (int i = 0; i < 3; i++) {
    snprintf(s[i].url, sizeof s[i].url, "https://exemplo.invalid/%d", i);
    strcpy(s[i].provedor, "Addon A"); s[i].fileIdx = -1;
  }
  stream_definir_lista(NULL, 0);
  stream_lista_acrescentar(s, 2, 2); // B e C chegam primeiro
  stream_lista_acrescentar(&s[2], 1, 0); // A chega depois, mas aparece antes
  assert(stream_proxima_vod(2) == 0);
  // Nunca abriu nenhuma (indice -1): comeca do topo, pulando a morta.
  fontesmortas_marcar(&s[2]);
  assert(stream_proxima_vod(-1) == 0);
  fontesmortas_limpar(&s[2]);
  fontesmortas_marcar(&s[0]);
  assert(stream_proxima_vod(2) == 1);
  assert(stream_proxima_vod(1) == -1);
  fontesmortas_limpar(&s[0]);
  assert(stream_proxima_vod(2) == 0);
  free(s); free(disco);
  puts("fontesmortas: PASS (identidade, TTL, disco, ordem, escolha manual)");
  return 0;
}
