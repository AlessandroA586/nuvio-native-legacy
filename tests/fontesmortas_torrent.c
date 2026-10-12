// Ciclo real do job manual: baixar -> resolver -> primeiro quadro -> limpar.
#define SDL_MAIN_HANDLED
#include "../src/app.c"
#include "fontesmortas.h"
#include <assert.h>

static Stream fonte;
static char *disco;
static Uint32 ticks = 1000;
static int pronto, abriu, erros;
char *dados_ler(const char *nome) { (void)nome; return disco ? strdup(disco) : NULL; }
int dados_gravar(const char *nome, const char *s) {
  (void)nome; free(disco); disco = strdup(s); return 1;
}
Uint32 SDL_GetTicks(void) { return ticks; }
int player_aberto(void) { return 1; }
int player_quer_sair(void) { return 0; }
const char *player_id_canal(void) { return ""; }
int player_indice(void) { return -1; }
int player_fonte_falhou(void) { return 0; }
int player_carregando(void) { return !pronto; }
void player_definir_fonte(const char *u) { assert(!strcmp(u, "https://video.invalid/resolvido")); abriu++; }
void player_erro_fonte(void) { erros++; }
void player_erro_fonte_motivo(const char *a, const char *b) { (void)a; (void)b; erros++; }
int video_pronto(void) { return pronto; }
double video_pos(void) { return pronto ? 1.0 : 0.0; }
int video_falhou(void) { return 0; }
const char *video_erro_texto(void) { return ""; }
int stream_atual(void) { return 0; }
unsigned stream_lista_geracao(void) { return 1; }
void stream_vod_falhou(int i) { assert(i == 0); fontesmortas_marcar(&fonte); }
void stream_vod_tocou(int i) { assert(i == 0); fontesmortas_limpar(&fonte); }
int stream_resolver_escolhida(int i, unsigned g, char *u, unsigned n,
                              char *s, unsigned ns, int *pct) {
  (void)i; (void)g; (void)u; (void)n; (void)s; (void)ns; (void)pct;
  assert(!"nenhum pedido pendente"); return 0;
}
int p2p_ultimo_erro(void) { return 0; }
void p2pmotor_arquivo_atual(uint64_t *t, uint64_t *m, uint64_t *l) { *t = *m = *l = 0; }
int debrid_sem_plano(void) { return 0; }
const char *debrid_sem_plano_frase(int m) { (void)m; return ""; }
const CatItem *cat_item(int i) { (void)i; return NULL; }
void ilhasinais_debrid_baixando(const char *s, const char *t) { (void)s; (void)t; }
const char *i18n(const char *s) { return s; }

static void *concluido(void *u) { return u; }
static void concluir(int resultado) {
  torrentJob.indice = 0; torrentJob.geracaoLista = 1;
  torrentJob.sessao = torrentSessao; torrentJob.resultado = resultado;
  strcpy(torrentJob.url, "https://video.invalid/resolvido");
  strcpy(torrentJob.servico, "TorBox"); torrentJob.pct = 50;
  atomic_store(&torrentJob.estado, 2);
  assert(!pthread_create(&fioTorrent, NULL, concluido, NULL));
  fioTorrentVivo = 1;
  processarTorrentJob();
}
int main(void) {
  strcpy(fonte.provedor, "Torrentio"); strcpy(fonte.infoHash, "aabbccdd"); fonte.fileIdx = 2;
  concluir(DEBRID_BAIXANDO);
  assert(erros == 1 && fontesmortas_idade(&fonte) >= 0);
  ticks += 120000; // Alguns minutos depois, a pessoa escolhe novamente.
  concluir(1);
  assert(abriu == 1 && fontesmortas_idade(&fonte) >= 0); // Resolver nao prova reproducao.
  assert(fonteManualDesde == ticks && "torrent manual sem vigia do primeiro quadro");
  vigiarAberturaManual();
  assert(fontesmortas_idade(&fonte) >= 0);
  pronto = 1;
  vigiarAberturaManual();
  assert(!fonteManualDesde && fontesmortas_idade(&fonte) < 0);
  fontesmortas_iniciar(); assert(fontesmortas_idade(&fonte) < 0);
  free(disco);
  puts("fontesmortas: PASS (torrent baixando, tentativa manual, primeiro quadro, disco)");
}
