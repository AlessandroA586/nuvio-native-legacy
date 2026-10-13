// DTS / TrueHD -> Dolby Digital no .tpk da Samsung: o servidor HTTP local de
// onde o player da TV le o fluxo convertido. Ver dts_hls_tpk.h e
// docs/plans/dts-truehd-ac3-tizen-DESIGN.md (secao 4.1). O andaime e o do
// src/proxyts.c (127.0.0.1, porta livre, um fio por conexao, sessao por
// geracao); o conversor e o dts_remux.c.
//
// MODOS (ajuste de teste, um so fica depois da S95C):
//  1 HLS sob demanda: playlist VOD montada do indice de quadros-chave; cada
//    segmento sai do conversor quando o player pede. Seek, posicao e duracao
//    ficam com o player.
//  2/3 progressivo (TS, MKV, MP4): um fluxo so, reaberto a cada seek (?t=).
//  4 o HLS do modo 1 no IP da rede da TV, em vez de 127.0.0.1.
// CORPO (pergunta 3 do teste): chunked, Content-Length (o segmento inteiro e
// gerado antes; Range respondido da memoria) ou fechar a conexao no fim.
//
// REGISTRO: toda requisicao do player sai no log com o Range e o
// Connection dela, e a resposta com bytes, primeiro byte, tempo total e o
// gerador usado (continuou ou buscou). Nada de URL da fonte nem do token.
#include "dts_hls_tpk.h"
#include "dts_remux.h"
#include <stdio.h>
#include <string.h>

#ifndef NV_TPK
int dts_tpk_disponivel(void) { return 0; }
const char *dts_tpk_modo_nome(int m) { (void)m; return ""; }
const char *dts_tpk_corpo_nome(int c) { (void)c; return ""; }
int dts_tpk_abrir(const char *u, const char *h, int t, int s, int m, int c, double i, char *o, size_t n) {
  (void)u; (void)h; (void)t; (void)s; (void)m; (void)c; (void)i; if (o && n) o[0] = 0; return 0;
}
int dts_tpk_url_em(double s, char *o, size_t n) { (void)s; if (o && n) o[0] = 0; return 0; }
void dts_tpk_fechar(void) {}
int dts_tpk_estado(void) { return DTSTPK_NADA; }
const char *dts_tpk_erro(void) { return ""; }
double dts_tpk_duracao(void) { return 0; }
int dts_tpk_modo(void) { return -1; }
double dts_tpk_inicio_stream(void) { return -1; }
double dts_tpk_entregue_ate(void) { return -1; }
const char *dts_tpk_caminho(void) { return ""; }
int dts_tpk_sem_indice(void) { return 0; }
int dts_tpk_e_url(const char *u) { (void)u; return 0; }
int dts_tpk_registro_rede(char *u, size_t n) { (void)u; if (n) u[0] = 0; return 0; }
#else

#include "../registro.h"
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <math.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define SEG_ALVO_S       6.0           /* HLS: keyframes grouped to >= 6 s */
#define LEN_ALVO_BYTES   (32.0 * 1048576.0)  /* Content-Length: segment kept in RAM */
#define LEN_TETO         (160u << 20)  /* a Content-Length segment never grows past this */
#define ESPERA_SONDA_MS  30000
#define ESPERA_GERADOR_MS 30000
#define GERADORES        2

static const char *const MODO_NOME[DTSTPK_MODOS] = {
  "HLS 127.0.0.1", "progressive TS", "progressive MKV", "progressive MP4", "HLS LAN IP", "off" };
static const char *const CORPO_NOME[DTSTPK_CORPOS] = { "chunked", "content-length", "close" };
const char *dts_tpk_modo_nome(int m) { return m >= 0 && m < DTSTPK_MODOS ? MODO_NOME[m] : "?"; }
const char *dts_tpk_corpo_nome(int c) { return c >= 0 && c < DTSTPK_CORPOS ? CORPO_NOME[c] : "?"; }

static uint64_t agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (uint64_t)t.tv_sec * 1000u + (uint64_t)t.tv_nsec / 1000000u;
}
static void prazoEm(struct timespec *ts, int ms) {
  clock_gettime(CLOCK_REALTIME, ts);
  ts->tv_sec += ms / 1000; ts->tv_nsec += (long)(ms % 1000) * 1000000L;
  if (ts->tv_nsec >= 1000000000L) { ts->tv_nsec -= 1000000000L; ts->tv_sec++; }
}

// --- token e IP -------------------------------------------------------------
static void gerarToken(char *t, int n) {
  static const char A[] = "abcdefghjkmnpqrstuvwxyz23456789";
  unsigned char b[32];
  int f = open("/dev/urandom", O_RDONLY), k = 0;
  ssize_t r = f >= 0 ? read(f, b, sizeof b) : -1;
  if (f >= 0) close(f);
  if (r < (ssize_t)sizeof b) {   // sem urandom: relogio (o token so separa sessoes)
    uint64_t x = agoraMs() * 6364136223846793005ULL + (uint64_t)getpid();
    for (int i = 0; i < (int)sizeof b; i++) { x = x * 6364136223846793005ULL + 1442695040888963407ULL; b[i] = (unsigned char)(x >> 33); }
  }
  for (int i = 0; i < (int)sizeof b && k < n; i++) if (b[i] < 248) t[k++] = A[b[i] % 31];
  t[k] = 0;
}

static int privado(in_addr_t a) {
  unsigned long h = ntohl(a);
  return (h >> 24) == 10 || (h >> 20) == 0xAC1 || (h >> 16) == 0xC0A8;
}
// Mesma regra do celular.c: a interface da rota padrao, senao a primeira privada.
static int ipLan(char *dst, size_t n) {
  struct sockaddr_in alvo, eu;
  socklen_t tam = sizeof eu;
  struct ifaddrs *lista = NULL, *i;
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  in_addr_t achou = 0;
  if (s >= 0) {
    memset(&alvo, 0, sizeof alvo);
    alvo.sin_family = AF_INET; alvo.sin_port = htons(9);
    inet_pton(AF_INET, "192.0.2.1", &alvo.sin_addr);
    if (!connect(s, (struct sockaddr *)&alvo, sizeof alvo) &&
        !getsockname(s, (struct sockaddr *)&eu, &tam) && privado(eu.sin_addr.s_addr))
      achou = eu.sin_addr.s_addr;
    close(s);
  }
  if (!achou && !getifaddrs(&lista)) {
    for (i = lista; i; i = i->ifa_next) {
      if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET) continue;
      if (!(i->ifa_flags & IFF_UP) || (i->ifa_flags & IFF_LOOPBACK)) continue;
      if (privado(((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr)) {
        achou = ((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr; break;
      }
    }
    freeifaddrs(lista);
  }
  if (!achou) return 0;
  struct in_addr a; a.s_addr = achou;
  return inet_ntop(AF_INET, &a, dst, (socklen_t)n) != NULL;
}

// --- sockets ------------------------------------------------------------------
static int enviarTudo(int s, const void *b, size_t n, volatile int *parar1, volatile int *parar2) {
  const char *p = b;
  while (n) {
    ssize_t r = send(s, p, n, MSG_NOSIGNAL);
    if (r < 0 && (errno == EINTR)) continue;
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      // Player parado (pausa, buffer cheio): espera sem prazo, so o cancelamento solta.
      if ((parar1 && *parar1) || (parar2 && *parar2)) return 0;
      continue;
    }
    if (r <= 0) return 0;
    p += r; n -= (size_t)r;
  }
  return 1;
}

static int ouvir(in_addr_t onde, int portaPreferida, int *porta) {
  struct sockaddr_in a;
  socklen_t al = sizeof a;
  int um = 1, s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) return -1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(onde);
  a.sin_port = htons((unsigned short)portaPreferida);
  if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) {
    if (!portaPreferida) { close(s); return -1; }
    a.sin_port = 0;
    if (bind(s, (struct sockaddr *)&a, sizeof a) < 0) { close(s); return -1; }
  }
  if (listen(s, 16) < 0 || getsockname(s, (struct sockaddr *)&a, &al) < 0) { close(s); return -1; }
  *porta = ntohs(a.sin_port);
  return s;
}

// Le o cabecalho do pedido (ate a linha vazia). 0 = conexao fechada/prazo.
static int lerPedido(int s, char *b, size_t n, int prazoMs) {
  size_t tot = 0;
  uint64_t fim = agoraMs() + (uint64_t)prazoMs;
  while (tot < n - 1) {
    struct pollfd p = { s, POLLIN, 0 };
    int restam = (int)(fim > agoraMs() ? fim - agoraMs() : 0);
    if (restam <= 0 || poll(&p, 1, restam) <= 0) return 0;
    ssize_t r = recv(s, b + tot, n - 1 - tot, 0);
    if (r <= 0) return 0;
    tot += (size_t)r; b[tot] = 0;
    if (strstr(b, "\r\n\r\n") || strstr(b, "\n\n")) return (int)tot;
  }
  return 0;
}

typedef struct {
  char metodo[8], caminho[400], query[128], range[64], conexao[32], agente[96];
  int http11, head;
} Pedido;

static void campo(const char *cab, const char *nome, char *dst, size_t n) {
  const char *p = cab;
  size_t k = strlen(nome);
  dst[0] = 0;
  while ((p = strchr(p, '\n')) != NULL) {
    p++;
    if (!strncasecmp(p, nome, k) && p[k] == ':') {
      const char *v = p + k + 1, *f;
      while (*v == ' ' || *v == '\t') v++;
      f = v + strcspn(v, "\r\n");
      snprintf(dst, n, "%.*s", (int)(f - v), v);
      return;
    }
  }
}

static int lerLinha(const char *cab, Pedido *p) {
  char alvo[400];
  memset(p, 0, sizeof *p);
  if (sscanf(cab, "%7s %399s", p->metodo, alvo) != 2) return 0;
  p->http11 = strstr(cab, "HTTP/1.1\r") != NULL || strstr(cab, "HTTP/1.1\n") != NULL;
  p->head = !strcmp(p->metodo, "HEAD");
  { char *q = strchr(alvo, '?');
    if (q) { snprintf(p->query, sizeof p->query, "%s", q + 1); *q = 0; }
    snprintf(p->caminho, sizeof p->caminho, "%s", alvo); }
  campo(cab, "Range", p->range, sizeof p->range);
  campo(cab, "Connection", p->conexao, sizeof p->conexao);
  campo(cab, "User-Agent", p->agente, sizeof p->agente);
  return !strcmp(p->metodo, "GET") || p->head;
}

static void responderSimples(int s, int cod, const char *msg, const char *tipo, const char *corpo, int manter) {
  char cab[512];
  size_t n = corpo ? strlen(corpo) : 0;
  snprintf(cab, sizeof cab, "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
           "Cache-Control: no-cache\r\nConnection: %s\r\n\r\n", cod, msg, tipo ? tipo : "text/plain", n,
           manter ? "keep-alive" : "close");
  enviarTudo(s, cab, strlen(cab), NULL, NULL);
  if (n) enviarTudo(s, corpo, n, NULL, NULL);
}

// --- sessao -----------------------------------------------------------------
typedef struct {
  DtsRemuxGen *g;
  int ocupado, seg;      // seg em producao (-2 = progressivo), quando ocupado
  volatile int parar;
  unsigned uso;
} Gerador;

typedef struct Sessao {
  unsigned id;
  char token[12];
  char *url, *cabs;
  int track, stereo, modo, corpo;
  volatile int cancelar;
  pthread_mutex_t m;
  pthread_cond_t c;
  int refs;                       // sob a trava global
  int estado, semIndice;
  char erro[160], caminho[40];
  double duracao;
  DtsRemux *rx;
  Gerador ger[GERADORES];
  unsigned uso;
  double inicioStream;            // progressivo: keyframe do fluxo em curso
  double entregueAte;             // dts_tpk_entregue_ate: fim do video ja enviado ao player
  // Content-Length: o ultimo segmento fica na memoria para o Range
  int cacheSeg; unsigned char *cache; size_t cacheN;
  // resumo
  unsigned nPedidos, nSegs, nContinuou, nBuscou, nErros;
  int64_t bytes;
  uint64_t criadaMs, prontaMs, primeiroPedidoMs;
  char agente[96];
} Sessao;

static pthread_mutex_t G = PTHREAD_MUTEX_INITIALIZER;
static Sessao *atual;
static unsigned contador;
static int sockLo = -1, portaLo, sockLan = -1, portaLan;
static char ipLanAtual[48];

static void sessaoDestruir(Sessao *s) {
  for (int i = 0; i < GERADORES; i++) dts_remux_gen_free(s->ger[i].g);
  dts_remux_close(s->rx);
  free(s->cache); free(s->url); free(s->cabs);
  pthread_cond_destroy(&s->c); pthread_mutex_destroy(&s->m);
  free(s);
}
// Fechar geradores junta fios de rede (curl cancelado responde rapido, mas um
// DNS preso nao): sempre fora do fio de quem soltou a ultima referencia.
static void *fioDestruir(void *u) { sessaoDestruir(u); return NULL; }
static void sessaoSoltar(Sessao *s) {
  int fim;
  pthread_mutex_lock(&G);
  fim = --s->refs == 0;
  pthread_mutex_unlock(&G);
  if (fim) {
    pthread_t t;
    if (pthread_create(&t, NULL, fioDestruir, s) == 0) pthread_detach(t);
    else sessaoDestruir(s);
  }
}
static Sessao *sessaoPegar(const char *token) {
  Sessao *s = NULL;
  pthread_mutex_lock(&G);
  if (atual && token && !strcmp(atual->token, token)) { s = atual; s->refs++; }
  pthread_mutex_unlock(&G);
  return s;
}

static void sessaoResumo(Sessao *s) {
  printf("[dts-tpk] sessao %u fim: %u pedido(s), %u segmento(s) (%u continuaram, %u buscaram), "
         "%u erro(s), %.1f MB enviados%s%s\n", s->id, s->nPedidos, s->nSegs, s->nContinuou, s->nBuscou,
         s->nErros, s->bytes / 1048576.0, s->agente[0] ? "; player: " : "", s->agente);
  fflush(stdout);
}

static void *fioSonda(void *u) {
  Sessao *s = u;
  char err[160] = "";
  DtsRemux *rx = dts_remux_open(s->url, s->cabs, s->track, s->stereo, SEG_ALVO_S, &s->cancelar, err, sizeof err);
  const DtsRemuxInfo *in = rx ? dts_remux_info(rx) : NULL;
  if (rx && s->corpo == DTSTPK_CORPO_LENGTH && in->size_bytes > 0 && in->duration_s > 1.0) {
    // O segmento inteiro fica na memoria antes de sair: alvo de ~32 MB.
    double bps = in->size_bytes / in->duration_s, alvo = LEN_ALVO_BYTES / bps;
    if (alvo < 2.0) alvo = 2.0;
    if (alvo > SEG_ALVO_S) alvo = SEG_ALVO_S;
    dts_remux_resegment(rx, alvo);
  }
  pthread_mutex_lock(&s->m);
  if (s->cancelar) { if (rx) dts_remux_close(rx); rx = NULL; s->estado = DTSTPK_FALHOU; snprintf(s->erro, sizeof s->erro, "cancelled"); }
  else if (!rx) { s->estado = DTSTPK_FALHOU; snprintf(s->erro, sizeof s->erro, "%s", err[0] ? err : "probe failed"); }
  else if ((s->modo == DTSTPK_HLS || s->modo == DTSTPK_HLS_LAN) && (in->n_segments < 1 || !in->index_complete)) {
    // HLS precisa do indice inteiro (a playlist sai dele): sem Cues nao ha HLS.
    // video_tpk.c refaz no TS continuo, que busca por tempo.
    s->estado = DTSTPK_FALHOU; s->semIndice = 1;
    snprintf(s->erro, sizeof s->erro, "no keyframe index to the end (no Cues): HLS impossible");
  } else if ((s->modo == DTSTPK_HLS || s->modo == DTSTPK_HLS_LAN) && fabs(in->fps - 24.0) < 0.002) {
    // 24.000 fps EXATOS (nao 23,976): no HLS o player da S95C anda aos trancos
    // o filme inteiro; o mesmo arquivo no TS continuo anda liso (Round 2). Mesmo
    // caminho do "sem indice": o app refaz no TS continuo.
    s->estado = DTSTPK_FALHOU; s->semIndice = 1;
    snprintf(s->erro, sizeof s->erro, "24.000 fps: HLS judders on this TV, continuous TS instead");
  } else {
    s->rx = rx; s->estado = DTSTPK_PRONTO; s->duracao = in->duration_s;
    snprintf(s->caminho, sizeof s->caminho, "%s", in->decoder_path);
  }
  s->prontaMs = agoraMs();
  pthread_cond_broadcast(&s->c);
  pthread_mutex_unlock(&s->m);
  if (rx) {
    char fpsTxt[24] = "?";
    if (in->fps > 0) { long f = lround(in->fps * 1000.0); snprintf(fpsTxt, sizeof fpsTxt, "%ld.%03ld", f / 1000, f % 1000); }
    printf("[dts-tpk] sessao %u fonte: %s %dx%d %s fps, dv=%s%.0d hdr(container)=%s, audio %s (audio %d do arquivo) %d ch %d Hz -> %s %d ch %d kb/s "
           "(%s); %d quadro(s)-chave, %d segmento(s) de ate %.1f s, %.0f s, %.0f MB; sonda %u ms\n",
           s->id, in->video_codec, in->width, in->height, fpsTxt, in->dv_profile < 0 ? "nao" : "perfil ",
           in->dv_profile < 0 ? 0 : in->dv_profile, in->hdr, in->audio_codec, in->audio_ordinal, in->in_channels,
           in->in_rate, in->out_codec, in->out_channels, in->out_bitrate / 1000, in->decoder_path, in->n_keyframes,
           in->n_segments, in->max_segment_s, in->duration_s, in->size_bytes > 0 ? in->size_bytes / 1048576.0 : -1.0,
           (unsigned)(s->prontaMs - s->criadaMs));
    if (s->semIndice) {
      printf("[dts-tpk] sessao %u: %s: o app troca para o TS continuo\n", s->id, s->erro);
      dts_remux_close(rx);             // a sessao nao ficou com ele
    }
  } else if (!s->cancelar)
    printf("[dts-tpk] sessao %u: a fonte nao abriu no conversor: %s (%u ms)\n", s->id, s->erro,
           (unsigned)(s->prontaMs - s->criadaMs));
  fflush(stdout);
  sessaoSoltar(s);
  return NULL;
}

// Espera a sonda. 1 = pronta; 0 = falhou/cancelada (com resposta enviada).
static int esperarPronta(Sessao *s, int sock, int manter) {
  struct timespec ts;
  prazoEm(&ts, ESPERA_SONDA_MS);
  pthread_mutex_lock(&s->m);
  while (s->estado == DTSTPK_SONDANDO && !s->cancelar)
    if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) break;
  // HLS sem indice: o app refaz no TS continuo e fecha esta sessao. O player
  // fica sem resposta ate la (um 503 viraria erro do player, que chega depois
  // da troca e derrubaria o fluxo novo).
  if (s->semIndice) {
    prazoEm(&ts, 15000);
    while (!s->cancelar)
      if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) break;
  }
  int ok = s->estado == DTSTPK_PRONTO && !s->cancelar, calar = s->semIndice && s->cancelar;
  pthread_mutex_unlock(&s->m);
  if (!ok && !calar) responderSimples(sock, 503, "Service Unavailable", NULL, "converter not ready\n", manter);
  return ok;
}

// --- geradores ----------------------------------------------------------------
// Ordem (design 4.1): (1) um ocioso que CONTINUA em `seg`; (2) se outro esta
// fazendo seg-1, espera por ele (o player pediu dois em paralelo: continuar
// custa nada, buscar baixaria o mesmo trecho duas vezes); (3) um ocioso
// qualquer (busca); (4) um novo, ate GERADORES; (5) para o mais antigo.
static Gerador *pegarGerador(Sessao *s, int seg, int *continua, char *err, size_t errn) {
  struct timespec ts;
  Gerador *esc = NULL;
  int i;
  *continua = 0;
  prazoEm(&ts, ESPERA_GERADOR_MS);
  pthread_mutex_lock(&s->m);
  while (!esc && !s->cancelar) {
    int esperar = 0, vaga = -1;
    for (i = 0; i < GERADORES && seg >= 0; i++)
      if (s->ger[i].g && !s->ger[i].ocupado && dts_remux_gen_next(s->ger[i].g) == seg) { esc = &s->ger[i]; *continua = 1; break; }
    if (esc) break;
    for (i = 0; i < GERADORES && seg > 0; i++)
      if (s->ger[i].ocupado && s->ger[i].seg == seg - 1 && !s->ger[i].parar) esperar = 1;
    if (esperar) {
      if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) { snprintf(err, errn, "timed out waiting for segment %d", seg - 1); break; }
      continue;
    }
    for (i = 0; i < GERADORES; i++)
      if (s->ger[i].g && !s->ger[i].ocupado && (!esc || s->ger[i].uso < esc->uso)) esc = &s->ger[i];
    if (esc) break;
    // Progressivo: o player le UM fluxo por vez. Pedido novo (seek, ou o player
    // reconectando) com o anterior ainda ocupado: o anterior para e este fica
    // com o leitor dele, em vez de abrir outro (cada leitor novo e outra
    // conexao na fonte e reler o cabecalho; foi isso que o TorBox recusou).
    if (seg < 0) {
      Gerador *prog = NULL;
      for (i = 0; i < GERADORES; i++) if (s->ger[i].g && s->ger[i].ocupado && s->ger[i].seg == -2) prog = &s->ger[i];
      if (prog) {
        if (!prog->parar) {
          prog->parar = 1;
          dts_remux_gen_interrupt(prog->g);
          printf("[dts-tpk] sessao %u: fluxo anterior para; o pedido novo usa o mesmo leitor\n", s->id);
          fflush(stdout);
        }
        if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) { snprintf(err, errn, "previous stream did not stop"); break; }
        continue;
      }
    }
    for (i = 0; i < GERADORES; i++) if (!s->ger[i].g && !s->ger[i].ocupado) { vaga = i; break; }
    if (vaga >= 0) {
      Gerador *nv = &s->ger[vaga];
      DtsRemuxGen *g;
      nv->ocupado = 1; nv->seg = seg;      // reserva a vaga enquanto abre a fonte
      pthread_mutex_unlock(&s->m);
      g = dts_remux_gen_new(s->rx, err, errn);
      pthread_mutex_lock(&s->m);
      nv->g = g; nv->ocupado = 0;
      pthread_cond_broadcast(&s->c);
      if (!g) break;
      printf("[dts-tpk] sessao %u: gerador %d pronto\n", s->id, vaga);
      fflush(stdout);
      esc = nv;
      break;
    }
    // Todos ocupados: o mais antigo larga o que faz (o player desistiu dele).
    { Gerador *velho = NULL;
      for (i = 0; i < GERADORES; i++)
        if (s->ger[i].ocupado && s->ger[i].g && (!velho || s->ger[i].uso < velho->uso)) velho = &s->ger[i];
      if (velho && !velho->parar) {
        velho->parar = 1;
        dts_remux_gen_interrupt(velho->g);   // preso na rede: solta ja, nao no proximo pacote
        printf("[dts-tpk] sessao %u: %d geradores ocupados, o mais antigo (seg %d) para\n", s->id, GERADORES, velho->seg);
        fflush(stdout);
      } }
    if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) { snprintf(err, errn, "no free generator"); break; }
  }
  if (esc && !s->cancelar) { esc->ocupado = 1; esc->seg = seg; esc->parar = 0; esc->uso = ++s->uso; }
  else esc = NULL;
  pthread_mutex_unlock(&s->m);
  if (!esc && !err[0]) snprintf(err, errn, "session closed");
  return esc;
}

static void soltarGerador(Sessao *s, Gerador *g) {
  pthread_mutex_lock(&s->m);
  g->ocupado = 0; g->seg = -1; g->parar = 0;
  pthread_cond_broadcast(&s->c);
  pthread_mutex_unlock(&s->m);
}

// --- saida de um run ------------------------------------------------------------
typedef struct {
  int sock, corpo, manter, cabEnviado, status;
  const char *tipo;
  Sessao *s;
  Gerador *ger;
  int64_t bytes;
  uint64_t t0, primeiroByte;
  unsigned char *mem; size_t memN, memCap; int estourou;
} Saida;

static int enviarCab(Saida *o) {
  char cab[400];
  if (o->corpo == DTSTPK_CORPO_CHUNKED)
    snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nTransfer-Encoding: chunked\r\n"
             "Cache-Control: no-cache\r\nConnection: %s\r\n\r\n", o->tipo, o->manter ? "keep-alive" : "close");
  else
    snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", o->tipo);
  o->cabEnviado = 1; o->status = 200;
  return enviarTudo(o->sock, cab, strlen(cab), &o->s->cancelar, &o->ger->parar);
}

static int saidaEscrever(void *u, const unsigned char *d, int n) {
  Saida *o = u;
  if (!o->primeiroByte) o->primeiroByte = agoraMs();
  if (o->corpo == DTSTPK_CORPO_LENGTH) {
    if (o->memN + (size_t)n > o->memCap) {
      size_t cap = o->memCap ? o->memCap * 2 : (8u << 20);
      while (cap < o->memN + (size_t)n) cap *= 2;
      if (cap > LEN_TETO) { o->estourou = 1; return -1; }
      unsigned char *m = realloc(o->mem, cap);
      if (!m) { o->estourou = 1; return -1; }
      o->mem = m; o->memCap = cap;
    }
    memcpy(o->mem + o->memN, d, (size_t)n); o->memN += (size_t)n;
    return n;
  }
  if (!o->cabEnviado && !enviarCab(o)) return -1;
  if (o->corpo == DTSTPK_CORPO_CHUNKED) {
    char tam[16];
    int k = snprintf(tam, sizeof tam, "%x\r\n", (unsigned)n);
    if (!enviarTudo(o->sock, tam, (size_t)k, &o->s->cancelar, &o->ger->parar) ||
        !enviarTudo(o->sock, d, (size_t)n, &o->s->cancelar, &o->ger->parar) ||
        !enviarTudo(o->sock, "\r\n", 2, &o->s->cancelar, &o->ger->parar)) return -1;
  } else if (!enviarTudo(o->sock, d, (size_t)n, &o->s->cancelar, &o->ger->parar)) return -1;
  o->bytes += n;
  { double e = dts_remux_gen_emitted_s(o->ger->g);
    if (e >= 0) { pthread_mutex_lock(&o->s->m); o->s->entregueAte = e; pthread_mutex_unlock(&o->s->m); } }
  return n;
}

// Corpo da memoria (Content-Length), com Range "bytes=a-b" / "bytes=a-".
static void enviarMemoria(int sock, Sessao *s, const char *tipo, const unsigned char *b, size_t n,
                          const char *range, int manter, int *status, int64_t *enviados) {
  char cab[400];
  size_t ini = 0, fim = n ? n - 1 : 0;
  int parcial = 0;
  unsigned long long a, z;
  if (range[0] && sscanf(range, "bytes=%llu-%llu", &a, &z) >= 1) {
    if (sscanf(range, "bytes=%llu-%llu", &a, &z) < 2) z = n ? n - 1 : 0;
    if (a >= n) {
      snprintf(cab, sizeof cab, "HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */%zu\r\n"
               "Content-Length: 0\r\nConnection: %s\r\n\r\n", n, manter ? "keep-alive" : "close");
      enviarTudo(sock, cab, strlen(cab), &s->cancelar, NULL);
      *status = 416; return;
    }
    ini = (size_t)a; fim = z >= n ? n - 1 : (size_t)z; parcial = 1;
  }
  if (parcial)
    snprintf(cab, sizeof cab, "HTTP/1.1 206 Partial Content\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
             "Content-Range: bytes %zu-%zu/%zu\r\nAccept-Ranges: bytes\r\nCache-Control: no-cache\r\nConnection: %s\r\n\r\n",
             tipo, fim - ini + 1, ini, fim, n, manter ? "keep-alive" : "close");
  else
    snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\nAccept-Ranges: bytes\r\n"
             "Cache-Control: no-cache\r\nConnection: %s\r\n\r\n", tipo, n, manter ? "keep-alive" : "close");
  *status = parcial ? 206 : 200;
  if (enviarTudo(sock, cab, strlen(cab), &s->cancelar, NULL) && n && enviarTudo(sock, b + ini, fim - ini + 1, &s->cancelar, NULL))
    *enviados = (int64_t)(fim - ini + 1);
}

// Um segmento (seg >= 0) ou o fluxo progressivo (seg < 0, a partir de `t`).
// Devolve se a conexao pode seguir aberta.
static int servirFluxo(int sock, Sessao *s, const Pedido *pd, unsigned nr, int seg, double t, const char *fmt,
                       const char *tipo, int manter) {
  char err[160] = "";
  int continua = 0, rr;
  uint64_t t0 = agoraMs();
  DtsRemuxStats st;
  Saida o;
  int corpo = s->corpo;
  if (seg < 0 && corpo == DTSTPK_CORPO_LENGTH) corpo = DTSTPK_CORPO_CLOSE;   // fluxo sem fim nao tem tamanho
  if (corpo == DTSTPK_CORPO_CLOSE) manter = 0;
  memset(&st, 0, sizeof st);
  // Content-Length: o mesmo segmento de novo (Range de retomada) sai da memoria.
  if (seg >= 0 && corpo == DTSTPK_CORPO_LENGTH) {
    pthread_mutex_lock(&s->m);
    if (s->cache && s->cacheSeg == seg) {
      unsigned char *b = s->cache; size_t n = s->cacheN;
      s->cache = NULL;                     // emprestado: volta depois de enviar
      pthread_mutex_unlock(&s->m);
      int status = 0; int64_t env = 0;
      if (!pd->head) enviarMemoria(sock, s, tipo, b, n, pd->range, manter, &status, &env);
      pthread_mutex_lock(&s->m);
      if (!s->cache) { s->cache = b; s->cacheN = n; s->cacheSeg = seg; } else free(b);
      s->bytes += env;
      pthread_mutex_unlock(&s->m);
      printf("[dts-tpk] #%u <- %d seg%d da memoria: %lld B em %u ms\n", nr, status, seg, (long long)env,
             (unsigned)(agoraMs() - t0));
      fflush(stdout);
      return manter;
    }
    pthread_mutex_unlock(&s->m);
  }
  if (pd->head) {
    // HEAD: so o cabecalho; o tamanho de um segmento nao existe antes de gera-lo.
    char cab[300];
    snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nCache-Control: no-cache\r\nConnection: %s\r\n\r\n",
             tipo, manter ? "keep-alive" : "close");
    enviarTudo(sock, cab, strlen(cab), &s->cancelar, NULL);
    printf("[dts-tpk] #%u <- 200 HEAD (sem corpo)\n", nr);
    fflush(stdout);
    return manter;
  }
  Gerador *ger = pegarGerador(s, seg < 0 ? -2 : seg, &continua, err, sizeof err);
  if (!ger) {
    responderSimples(sock, 503, "Service Unavailable", NULL, "busy\n", 0);
    printf("[dts-tpk] #%u <- 503: %s\n", nr, err);
    fflush(stdout);
    pthread_mutex_lock(&s->m); s->nErros++; pthread_mutex_unlock(&s->m);
    return 0;
  }
  if (seg < 0) {   // o player conta a posicao a partir do quadro-chave onde o fluxo comeca
    pthread_mutex_lock(&s->m); s->inicioStream = dts_remux_keyframe_before(s->rx, t); pthread_mutex_unlock(&s->m);
  }
  memset(&o, 0, sizeof o);
  o.sock = sock; o.corpo = corpo; o.manter = manter; o.tipo = tipo; o.s = s; o.ger = ger; o.t0 = t0;
  if (pd->range[0] && strncmp(pd->range, "bytes=0-", 8) && corpo != DTSTPK_CORPO_LENGTH)
    printf("[dts-tpk] #%u: Range %s ignorado (corpo %s gera do inicio)\n", nr, pd->range, CORPO_NOME[corpo]);
  rr = dts_remux_gen_run(ger->g, seg, t, fmt, saidaEscrever, &o, &ger->parar, &st, err, sizeof err);
  soltarGerador(s, ger);
  int status = o.status, ok = rr == 0 && !o.estourou;
  int64_t enviados = o.bytes;
  if (ok && corpo == DTSTPK_CORPO_LENGTH) {
    enviarMemoria(sock, s, tipo, o.mem, o.memN, pd->range, manter, &status, &enviados);
    pthread_mutex_lock(&s->m);
    free(s->cache); s->cache = o.mem; s->cacheN = o.memN; s->cacheSeg = seg; o.mem = NULL;
    pthread_mutex_unlock(&s->m);
  } else if (ok && corpo == DTSTPK_CORPO_CHUNKED) {
    if (!o.cabEnviado) enviarCab(&o);
    if (!enviarTudo(sock, "0\r\n\r\n", 5, &s->cancelar, NULL)) manter = 0;
  } else if (ok && !o.cabEnviado) enviarCab(&o);
  if (!ok) {
    if (!o.cabEnviado && rr != -2) {
      responderSimples(sock, o.estourou ? 507 : 500, o.estourou ? "Insufficient Storage" : "Internal Server Error",
                       NULL, "conversion failed\n", 0);
      status = o.estourou ? 507 : 500;
    }
    manter = 0;
  }
  free(o.mem);
  pthread_mutex_lock(&s->m);
  s->bytes += enviados;
  if (seg >= 0 && ok) { s->nSegs++; if (!st.fresh) s->nContinuou++; else s->nBuscou++; }
  if (!ok && rr != -2) s->nErros++;
  pthread_mutex_unlock(&s->m);
  uint64_t fim = agoraMs();
  char stTxt[8];                           // "-": nada saiu (o player fechou antes)
  if (status) snprintf(stTxt, sizeof stTxt, "%d", status); else snprintf(stTxt, sizeof stTxt, "-");
  if (seg >= 0)
    printf("[dts-tpk] #%u <- %s seg%d (%.3f-%.3f s): %lld B, 1o byte %u ms, total %u ms, gerador %d %s, cpu %.0f ms "
           "(audio %.0f), rede %.0f ms%s%s\n", nr, stTxt, seg, st.first_pts_s, st.last_pts_s, (long long)enviados,
           (unsigned)((o.primeiroByte ? o.primeiroByte : fim) - t0), (unsigned)(fim - t0), (int)(ger - s->ger),
           st.fresh ? "buscou" : "continuou", st.cpu_s * 1000.0, st.audio_cpu_s * 1000.0, st.range_wait_s * 1000.0,
           ok ? "" : rr == -2 ? ", player fechou" : ", ERRO: ", ok || rr == -2 ? "" : err);
  else
    printf("[dts-tpk] #%u <- %s %s desde %.3f s: %lld B em %u ms, %s%s\n", nr, stTxt, fmt, dts_remux_gen_start_s(ger->g),
           (long long)enviados, (unsigned)(fim - t0), ok ? "fim do arquivo" : rr == -2 ? "player fechou" : "ERRO: ",
           ok || rr == -2 ? "" : err);
  fflush(stdout);
  return ok && manter;
}

static void playlist(Sessao *s, char *b, size_t n) {
  const DtsRemuxInfo *in = dts_remux_info(s->rx);
  int k, alvo = (int)ceil(in->max_segment_s);
  size_t p = 0;
  if (alvo < 1) alvo = 1;
  p += (size_t)snprintf(b + p, n - p, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:0\n"
                        "#EXT-X-PLAYLIST-TYPE:VOD\n#EXT-X-INDEPENDENT-SEGMENTS\n", alvo);
  for (k = 0; k < in->n_segments && p < n - 64; k++) {
    // Milissegundos inteiros: o LC_NUMERIC da TV nunca vira virgula aqui.
    long ms = lround(dts_remux_segment_duration(s->rx, k) * 1000.0);
    p += (size_t)snprintf(b + p, n - p, "#EXTINF:%ld.%03ld,\nseg%d.ts\n", ms / 1000, ms % 1000, k);
  }
  snprintf(b + p, n - p, "#EXT-X-ENDLIST\n");
}

// MASTER PLAYLIST: a single variant, but it tells the player the frame rate,
// size and HDR range before the first segment (the media playlist cannot). No
// CODECS: a wrong string is worse than none. Integers only: the TV's
// LC_NUMERIC never turns the decimal point into a comma here.
static void playlistMestre(Sessao *s, char *b, size_t n) {
  const DtsRemuxInfo *in = dts_remux_info(s->rx);
  long bw = in->size_bytes > 0 && in->duration_s > 1 ? (long)(in->size_bytes * 8.0 / in->duration_s) : 20000000L;
  long fps = lround(in->fps * 1000.0);
  size_t p = 0;
  if (bw < 1000000L) bw = 1000000L;
  p += (size_t)snprintf(b + p, n - p, "#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-INDEPENDENT-SEGMENTS\n#EXT-X-STREAM-INF:BANDWIDTH=%ld", bw);
  if (in->width > 0 && in->height > 0) p += (size_t)snprintf(b + p, n - p, ",RESOLUTION=%dx%d", in->width, in->height);
  if (fps > 0) p += (size_t)snprintf(b + p, n - p, ",FRAME-RATE=%ld.%03ld", fps / 1000, fps % 1000);
  if (!strncmp(in->hdr, "PQ", 2) || !strcmp(in->hdr, "HLG"))
    p += (size_t)snprintf(b + p, n - p, ",VIDEO-RANGE=%s", strncmp(in->hdr, "PQ", 2) ? "HLG" : "PQ");
  snprintf(b + p, n - p, "\nv.m3u8\n");
}

static int atenderPedido(int sock, const Pedido *pd, int manter) {
  char tok[16] = "", resto[200] = "";
  const char *c = pd->caminho;
  if (*c == '/') c++;
  { const char *b = strchr(c, '/');
    if (!b || b - c >= (int)sizeof tok) { responderSimples(sock, 404, "Not Found", NULL, "", 0); return 0; }
    snprintf(tok, sizeof tok, "%.*s", (int)(b - c), c);
    snprintf(resto, sizeof resto, "%s", b + 1); }
  Sessao *s = sessaoPegar(tok);
  unsigned nr;
  if (!s) {
    responderSimples(sock, 404, "Not Found", NULL, "", 0);
    printf("[dts-tpk] pedido de sessao antiga/desconhecida: 404\n");
    fflush(stdout);
    return 0;
  }
  pthread_mutex_lock(&s->m);
  nr = ++s->nPedidos;
  if (!s->primeiroPedidoMs) s->primeiroPedidoMs = agoraMs();
  int logAgente = !s->agente[0] && pd->agente[0];
  if (logAgente) snprintf(s->agente, sizeof s->agente, "%s", pd->agente);
  pthread_mutex_unlock(&s->m);
  printf("[dts-tpk] #%u -> %s %s%s%s Range=%s Connection=%s (+%u ms desde a abertura)%s%s\n", nr, pd->metodo, resto,
         pd->query[0] ? "?" : "", pd->query, pd->range[0] ? pd->range : "-", pd->conexao[0] ? pd->conexao : "-",
         (unsigned)(agoraMs() - s->criadaMs), logAgente ? " UA=" : "", logAgente ? pd->agente : "");
  fflush(stdout);
  int r = 0;
  if (!esperarPronta(s, sock, manter)) {
    if (s->semIndice) printf("[dts-tpk] #%u <- sem resposta: o app troca para o TS continuo\n", nr);
    else printf("[dts-tpk] #%u <- 503: %s\n", nr, s->erro[0] ? s->erro : "not ready");
    fflush(stdout);
  } else if (!strcmp(resto, "v.m3u8") || !strcmp(resto, "m.m3u8")) {
    int mestre = resto[0] == 'm';
    if (dts_remux_info(s->rx)->n_segments < 1) {
      responderSimples(sock, 404, "Not Found", NULL, "no keyframe index\n", manter);
      printf("[dts-tpk] #%u <- 404: fonte sem indice, nao ha playlist\n", nr);
    } else {
      size_t cap = (size_t)dts_remux_info(s->rx)->n_segments * 32 + 512;
      char *b = malloc(cap);
      if (b) {
        if (mestre) playlistMestre(s, b, cap); else playlist(s, b, cap);
        if (pd->head) {
          char cab[300];
          snprintf(cab, sizeof cab, "HTTP/1.1 200 OK\r\nContent-Type: application/vnd.apple.mpegurl\r\n"
                   "Content-Length: %zu\r\nCache-Control: no-cache\r\nConnection: %s\r\n\r\n", strlen(b), manter ? "keep-alive" : "close");
          enviarTudo(sock, cab, strlen(cab), NULL, NULL);
        } else responderSimples(sock, 200, "OK", "application/vnd.apple.mpegurl", b, manter);
        if (mestre) { const char *v = strstr(b, "BANDWIDTH"); v = v ? v : "";
                      printf("[dts-tpk] #%u <- 200 playlist mestre: %.*s\n", nr, (int)strcspn(v, "\n"), v); }
        else printf("[dts-tpk] #%u <- 200 playlist: %d segmento(s), %zu B\n", nr, dts_remux_info(s->rx)->n_segments, strlen(b));
        free(b);
        r = manter;
      }
    }
    fflush(stdout);
  } else if (!strncmp(resto, "seg", 3)) {
    int k = atoi(resto + 3);
    if (k < 0 || k >= dts_remux_info(s->rx)->n_segments) {
      responderSimples(sock, 404, "Not Found", NULL, "", manter);
      printf("[dts-tpk] #%u <- 404 segmento %d fora da playlist\n", nr, k);
      fflush(stdout);
      r = manter;
    } else r = servirFluxo(sock, s, pd, nr, k, 0, "mpegts", "video/mp2t", manter);
  } else if (!strncmp(resto, "prog.", 5)) {
    const char *e = resto + 5, *fmt = !strcmp(e, "mkv") ? "matroska" : !strcmp(e, "mp4") ? "mp4" : "mpegts";
    const char *tipo = !strcmp(e, "mkv") ? "video/x-matroska" : !strcmp(e, "mp4") ? "video/mp4" : "video/mp2t";
    const char *q = strstr(pd->query, "t=");
    double t = q ? atof(q + 2) : 0.0;
    r = servirFluxo(sock, s, pd, nr, -1, t, fmt, tipo, manter);
  } else {
    responderSimples(sock, 404, "Not Found", NULL, "", manter);
    r = manter;
  }
  sessaoSoltar(s);
  return r;
}

static void *atender(void *u) {
  int sock = (int)(intptr_t)u;
  char cab[8192];
  for (int n = 0;; n++) {
    Pedido pd;
    if (!lerPedido(sock, cab, sizeof cab, n ? 60000 : 15000)) break;
    if (!lerLinha(cab, &pd)) { responderSimples(sock, 405, "Method Not Allowed", NULL, "", 0); break; }
    int manter = pd.http11 ? strcasecmp(pd.conexao, "close") != 0 : strcasecmp(pd.conexao, "keep-alive") == 0;
    if (!atenderPedido(sock, &pd, manter)) break;
  }
  close(sock);
  return NULL;
}

static void *aceitar(void *u) {
  int ls = (int)(intptr_t)u;
  for (;;) {
    int s = accept(ls, NULL, NULL);
    pthread_t t;
    if (s < 0) { if (errno == EINTR) continue; usleep(200 * 1000); continue; }
    { struct timeval tv = { 1, 0 };   // envio em fatias de 1 s: o cancelamento e checado entre elas
      setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv); }
#ifdef SO_NOSIGPIPE
    { int um = 1; setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &um, sizeof um); }
#endif
    if (pthread_create(&t, NULL, atender, (void *)(intptr_t)s) == 0) pthread_detach(t);
    else close(s);
  }
  return NULL;
}

// Sob G. lan = 0: 127.0.0.1; 1: todas as interfaces (o player usa o IP da rede).
static int subir(int lan) {
  int *ls = lan ? &sockLan : &sockLo, *porta = lan ? &portaLan : &portaLo;
  pthread_t t;
  if (*ls >= 0) return 1;
  signal(SIGPIPE, SIG_IGN);
  *ls = ouvir(lan ? INADDR_ANY : INADDR_LOOPBACK, 0, porta);
  if (*ls < 0) return 0;
  if (pthread_create(&t, NULL, aceitar, (void *)(intptr_t)*ls) != 0) { close(*ls); *ls = -1; return 0; }
  pthread_detach(t);
  printf("[dts-tpk] servidor ouvindo em %s:%d\n", lan ? "0.0.0.0" : "127.0.0.1", *porta);
  fflush(stdout);
  return 1;
}

// Versao maior da Tizen (system-info por dlopen, como gpunivel.c). -1 = nao leu.
static int tizenMaior(void) {
  static int v = -2;
  if (v == -2) {
    static const char *const libs[] = { "libcapi-system-info.so.0", "libcapi-system-info.so", NULL };
    int (*get)(const char *, char **) = NULL;
    void *h = NULL;
    char *t = NULL;
    v = -1;
    for (int i = 0; libs[i] && !h; i++) h = dlopen(libs[i], RTLD_NOW);
    if (h) *(void **)&get = dlsym(h, "system_info_get_platform_string");
    if (get && get("http://tizen.org/feature/platform.version", &t) == 0 && t) { v = atoi(t); free(t); }
  }
  return v;
}

// O escopo do plano e o pacote NuvioTpk (Tizen 8 e 9, a S95C). A mesma .so roda
// nos hosts 6/6.5/7, onde a conversao (e as linhas dela nos Ajustes) fica fora.
int dts_tpk_disponivel(void) {
  static int c = -1;   // os Ajustes perguntam a cada quadro: decide uma vez
  if (c < 0) {
    int t = tizenMaior();
    c = (t < 0 || t >= 8) && dts_remux_available();
    printf("[dts-tpk] conversao DTS/TrueHD %s (tizen %d, FFmpeg %s)\n", c ? "disponivel" : "indisponivel", t,
           dts_remux_available() ? "ligado" : "ausente");
    fflush(stdout);
  }
  return c;
}

static void encerrarAtual(void) {   // sob G
  Sessao *s = atual;
  if (!s) return;
  atual = NULL;
  pthread_mutex_lock(&s->m);
  s->cancelar = 1;
  for (int i = 0; i < GERADORES; i++) s->ger[i].parar = 1;
  pthread_cond_broadcast(&s->c);
  pthread_mutex_unlock(&s->m);
  if (s->rx) dts_remux_cancel(s->rx);
  sessaoResumo(s);
  if (--s->refs == 0) {
    pthread_t t;
    if (pthread_create(&t, NULL, fioDestruir, s) == 0) pthread_detach(t);
    else sessaoDestruir(s);
  }
}

static int urlDe(Sessao *s, double t, char *out, size_t n) {
  const char *host = s->modo == DTSTPK_HLS_LAN ? ipLanAtual : "127.0.0.1";
  int porta = s->modo == DTSTPK_HLS_LAN ? portaLan : portaLo;
  switch (s->modo) {
    case DTSTPK_HLS: case DTSTPK_HLS_LAN:
      snprintf(out, n, "http://%s:%d/%s/m.m3u8", host, porta, s->token); break;
    default: {
      long ms = lround((t > 0 ? t : 0) * 1000.0);
      snprintf(out, n, "http://%s:%d/%s/prog.%s?t=%ld.%03ld", host, porta, s->token,
               s->modo == DTSTPK_PROG_MKV ? "mkv" : s->modo == DTSTPK_PROG_MP4 ? "mp4" : "ts", ms / 1000, ms % 1000);
    }
  }
  return 1;
}

int dts_tpk_abrir(const char *url, const char *headers, int track, int stereo, int modo, int corpo,
                  double inicio, char *out, size_t n) {
  Sessao *s;
  pthread_t t;
  int lan = modo == DTSTPK_HLS_LAN;
  if (out && n) out[0] = 0;
  if (!url || !*url || !out || n < 64 || modo < 0 || modo >= DTSTPK_DESLIGADO || !dts_remux_available()) return 0;
  if (corpo < 0 || corpo >= DTSTPK_CORPOS) corpo = DTSTPK_CORPO_CHUNKED;
  pthread_mutex_lock(&G);
  encerrarAtual();
  if (lan && !ipLan(ipLanAtual, sizeof ipLanAtual)) {
    pthread_mutex_unlock(&G);
    printf("[dts-tpk] modo HLS no IP da rede: a TV nao tem IP de rede local\n");
    fflush(stdout);
    return 0;
  }
  if (!subir(lan)) {
    pthread_mutex_unlock(&G);
    printf("[dts-tpk] o servidor nao subiu em %s (%s)\n", lan ? "0.0.0.0" : "127.0.0.1", strerror(errno));
    fflush(stdout);
    return 0;
  }
  s = calloc(1, sizeof *s);
  if (!s) { pthread_mutex_unlock(&G); return 0; }
  pthread_mutex_init(&s->m, NULL); pthread_cond_init(&s->c, NULL);
  s->id = ++contador;
  gerarToken(s->token, 8);
  s->url = strdup(url); s->cabs = strdup(headers ? headers : "");
  s->track = track; s->stereo = stereo; s->modo = modo; s->corpo = corpo;
  s->estado = DTSTPK_SONDANDO; s->cacheSeg = -1; s->inicioStream = -1; s->entregueAte = -1;
  s->criadaMs = agoraMs();
  for (int i = 0; i < GERADORES; i++) s->ger[i].seg = -1;
  if (!s->url || !s->cabs) { pthread_mutex_unlock(&G); sessaoDestruir(s); return 0; }
  s->refs = 2;   // a sessao atual e o fio da sonda
  atual = s;
  if (pthread_create(&t, NULL, fioSonda, s) != 0) {
    atual = NULL; s->refs = 0; pthread_mutex_unlock(&G); sessaoDestruir(s); return 0;
  }
  pthread_detach(t);
  urlDe(s, inicio, out, n);
  pthread_mutex_unlock(&G);
  printf("[dts-tpk] sessao %u: modo %s, corpo %s, endereco %s, audio %d do arquivo, saida %s; player abre %s\n", s->id,
         MODO_NOME[modo], CORPO_NOME[corpo], lan ? ipLanAtual : "127.0.0.1", track, stereo ? "AAC 2.0" : "AC3",
          strstr(out, ".m3u8") ? "a playlist HLS" : "o fluxo progressivo");
  fflush(stdout);
  return 1;
}

int dts_tpk_url_em(double t, char *out, size_t n) {
  int ok = 0;
  if (out && n) out[0] = 0;
  pthread_mutex_lock(&G);
  if (atual && out && n >= 64) {
    ok = urlDe(atual, t, out, n);
    pthread_mutex_lock(&atual->m); atual->inicioStream = -1; atual->entregueAte = -1; pthread_mutex_unlock(&atual->m);
  }
  pthread_mutex_unlock(&G);
  return ok;
}

void dts_tpk_fechar(void) {
  pthread_mutex_lock(&G);
  encerrarAtual();
  pthread_mutex_unlock(&G);
}

#define LER_ATUAL(tipo, campo, padrao) \
  tipo v = padrao; pthread_mutex_lock(&G); \
  if (atual) { pthread_mutex_lock(&atual->m); v = atual->campo; pthread_mutex_unlock(&atual->m); } \
  pthread_mutex_unlock(&G); return v;
int dts_tpk_estado(void) { LER_ATUAL(int, estado, DTSTPK_NADA) }
double dts_tpk_duracao(void) { LER_ATUAL(double, duracao, 0.0) }
int dts_tpk_modo(void) { LER_ATUAL(int, modo, -1) }
double dts_tpk_inicio_stream(void) { LER_ATUAL(double, inicioStream, -1.0) }
double dts_tpk_entregue_ate(void) { LER_ATUAL(double, entregueAte, -1.0) }
const char *dts_tpk_erro(void) {
  static char b[160];
  b[0] = 0;
  pthread_mutex_lock(&G);
  if (atual) { pthread_mutex_lock(&atual->m); snprintf(b, sizeof b, "%s", atual->erro); pthread_mutex_unlock(&atual->m); }
  pthread_mutex_unlock(&G);
  return b;
}
int dts_tpk_sem_indice(void) { LER_ATUAL(int, semIndice, 0) }
const char *dts_tpk_caminho(void) {
  static char b[40];
  b[0] = 0;
  pthread_mutex_lock(&G);
  if (atual) { pthread_mutex_lock(&atual->m); snprintf(b, sizeof b, "%s", atual->caminho); pthread_mutex_unlock(&atual->m); }
  pthread_mutex_unlock(&G);
  return b;
}
int dts_tpk_e_url(const char *u) {
  char a[80] = "", b[80] = "";
  if (!u) return 0;
  pthread_mutex_lock(&G);
  if (portaLo) snprintf(a, sizeof a, "http://127.0.0.1:%d/", portaLo);
  if (portaLan && ipLanAtual[0]) snprintf(b, sizeof b, "http://%s:%d/", ipLanAtual, portaLan);
  pthread_mutex_unlock(&G);
  return (a[0] && !strncmp(u, a, strlen(a))) || (b[0] && !strncmp(u, b, strlen(b)));
}

// --- registro na rede ---------------------------------------------------------
// Sem `sdb shell` na TV de varejo: o log do app sai por HTTP na LAN, com token.
// GET /<token>/         = dts (o que o teste pede quase sempre)
// GET /<token>/log?n=N   ultimas N linhas (padrao 3000)
// GET /<token>/dts?n=N   so as linhas da conversao, do player, do audio e da
//                        escolha de fonte (padrao 5000): as do menu nao empurram
//                        a reproducao para fora
// GET /<token>/help      os enderecos
// URLs saem cortadas no host, como no painel de log da TV (registro.c).
static int sockReg = -1, portaReg;
static char tokenReg[12], urlReg[96];

static void mascarar(char *s) {
  char *p = s;
  while ((p = strstr(p, "://")) != NULL) {
    char *h = p + 3, *fim;
    while (*h && *h != '/' && *h != '?' && *h != ' ') h++;
    if (*h != '/' && *h != '?') { p = h; continue; }
    fim = h;
    while (*fim && *fim != ' ' && *fim != '\n') fim++;
    if (fim - h < 4) { p = fim; continue; }
    memmove(h + 4, fim, strlen(fim) + 1);
    memcpy(h, "/...", 4);
    p = h + 4;
  }
}

static int linhaDts(const char *l) {
  static const char *const M[] = { "[dts-tpk]", "[dts-range]", "[video] tpk", "[audio]", "[mkv]", "[dts]",
                                   "[video] faixas", "[video] reconex", "[video] reconect", "[video] conexao",
                                   "[fonte]", "[player] saida", "[tv] ", "[etapa-anterior]", NULL };
  for (int i = 0; M[i]; i++) if (strstr(l, M[i])) return 1;
  return 0;
}

static void servirRegistro(int sock, const char *caminho, const char *query) {
  const char *arq = registro_arquivo();
  int so = !caminho[0] || !strcmp(caminho, "dts"), n = so ? 5000 : 3000;
  const char *q = strstr(query, "n=");
  if (q) n = atoi(q + 2);
  if (n < 1) n = 1;
  if (n > 20000) n = 20000;
  if (strcmp(caminho, "log") && !so) {
    char b[700];
    snprintf(b, sizeof b, "Nuvio log (this TV)\n\n  %s            conversion, player, audio and source lines (= dts)\n"
             "  %sdts?n=5000  the same, last 5000 of them\n  %slog?n=3000  last lines of the whole app log\n",
             urlReg, urlReg, urlReg);
    responderSimples(sock, 200, "OK", "text/plain; charset=utf-8", b, 0);
    return;
  }
  FILE *f = arq ? fopen(arq, "rb") : NULL;
  if (!f) { responderSimples(sock, 404, "Not Found", NULL, "no log file\n", 0); return; }
  long tam, ini;
  fseek(f, 0, SEEK_END); tam = ftell(f);
  ini = tam > (8L << 20) ? tam - (8L << 20) : 0;
  char *b = malloc((size_t)(tam - ini) + 1);
  if (!b) { fclose(f); responderSimples(sock, 500, "Internal Server Error", NULL, "", 0); return; }
  fseek(f, ini, SEEK_SET);
  size_t lidos = fread(b, 1, (size_t)(tam - ini), f);
  fclose(f);
  b[lidos] = 0;
  // De tras para a frente ate juntar n linhas (filtradas, no modo dts).
  char **lin = malloc((size_t)n * sizeof *lin);
  int k = 0;
  char *p = b + lidos;
  while (lin && p > b && k < n) {
    char *fim = p;
    if (fim > b && fim[-1] == '\n') fim--;
    char *c = fim;
    while (c > b && c[-1] != '\n') c--;
    *fim = 0;
    if (!so || linhaDts(c)) lin[k++] = c;
    p = c;
  }
  size_t cap = 256;
  for (int i = 0; i < k; i++) cap += strlen(lin[i]) + 1;
  char *saida = malloc(cap);
  if (saida && lin) {
    size_t o = 0;
    for (int i = k - 1; i >= 0; i--) { size_t l = strlen(lin[i]); memcpy(saida + o, lin[i], l); o += l; saida[o++] = '\n'; }
    saida[o] = 0;
    mascarar(saida);
    responderSimples(sock, 200, "OK", "text/plain; charset=utf-8", saida, 0);
  } else responderSimples(sock, 500, "Internal Server Error", NULL, "", 0);
  free(saida); free(lin); free(b);
}

static void *atenderRegistro(void *u) {
  int sock = (int)(intptr_t)u;
  char cab[4096];
  Pedido pd;
  if (lerPedido(sock, cab, sizeof cab, 10000) && lerLinha(cab, &pd)) {
    const char *c = pd.caminho[0] == '/' ? pd.caminho + 1 : pd.caminho;
    size_t k = strlen(tokenReg);
    if (!strncmp(c, tokenReg, k) && (c[k] == '/' || !c[k])) servirRegistro(sock, c[k] ? c + k + 1 : "", pd.query);
    else responderSimples(sock, 404, "Not Found", NULL, "", 0);
  }
  close(sock);
  return NULL;
}

static void *aceitarRegistro(void *u) {
  (void)u;
  for (;;) {
    int s = accept(sockReg, NULL, NULL);
    pthread_t t;
    if (s < 0) { if (errno == EINTR) continue; usleep(200 * 1000); continue; }
    { struct timeval tv = { 10, 0 }; setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv); }
    if (pthread_create(&t, NULL, atenderRegistro, (void *)(intptr_t)s) == 0) pthread_detach(t);
    else close(s);
  }
  return NULL;
}

int dts_tpk_registro_rede(char *url, size_t n) {
  char ip[48];
  int ok = 0;
  if (url && n) url[0] = 0;
  pthread_mutex_lock(&G);
  if (sockReg < 0 && ipLan(ip, sizeof ip)) {
    pthread_t t;
    signal(SIGPIPE, SIG_IGN);
    sockReg = ouvir(INADDR_ANY, 8099, &portaReg);   // porta fixa quando livre: facil de lembrar
    if (sockReg >= 0 && pthread_create(&t, NULL, aceitarRegistro, NULL) == 0) {
      pthread_detach(t);
      gerarToken(tokenReg, 6);
      snprintf(urlReg, sizeof urlReg, "http://%s:%d/%s/", ip, portaReg, tokenReg);
      printf("[dts-tpk] registro na rede: %s\n", urlReg);
      fflush(stdout);
    } else if (sockReg >= 0) { close(sockReg); sockReg = -1; }
  }
  if (sockReg >= 0 && url && n) { snprintf(url, n, "%s", urlReg); ok = 1; }
  pthread_mutex_unlock(&G);
  return ok;
}
#endif
