// Simkl como fonte da retomada e destino do "+". Ver simkl.h: la estao as
// rotas (todas conferidas na documentacao oficial), o que ficou de fora e o
// modelo de fios.
#include "simkl.h"
#include "perfisessao.h"
#include "ajustes.h"
#include "simklauth.h"
#include "nuvem.h"
#include "rede.h"
#include "js.h"
#include "idioma.h"
#include "trakt.h"
#include "artemetahub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Optional imports belong to consumers: public declarations keep producers strong.
extern int trakt_enfeitar_lote_sessao(CatItem *, int, uint64_t) NV_PERFIL_OPCIONAL;

#define SMK_API "https://api.simkl.com"
// Retomada: o mesmo teto da fileira (CONT_MAX de descoberta.c) com folga para
// o "a seguir" disputar lugar com os pausados.
#define SMK_PLAY_MAX 64
// Plan to Watch conhecido, para a guarda do "-". 400 ids x 24 B = ~10 KB; a
// lista de "quero ver" de uma pessoa real cabe com sobra.
#define SMK_PTW_MAX 400

int simkl_ativo(void) { return simklauth_token()[0] != 0; }

const char *simkl_aviso_sem_vinculo(int querSimkl) {
  return (querSimkl && !simkl_ativo()) ? SIMKL_VINCULE : NULL;
}

// ------------------------------------------------------------ JSON por nivel

// Valor da chave `chave` na PROFUNDIDADE 1 do primeiro objeto de [ini,fim).
// js_texto para no primeiro "imdb" da faixa inteira; num item de playback de
// serie o bloco "episode" pode vir antes do "show", e num item de all-items o
// "show" e irmao de campos como "last_watched". Ler por nivel e o que garante
// que o titulo e o imdb sao DA OBRA. *vi/*vf delimitam o valor cru.
static int valorRaiz(const char *ini, const char *fim, const char *chave,
                     const char **vi, const char **vf) {
  const char *p = ini;
  size_t n = strlen(chave);
  int prof = 0;
  if (!p) return 0;
  if (!fim) fim = p + strlen(p);
  while (p < fim && *p != '{') p++;
  for (; p < fim && *p; p++) {
    if (*p == '"') {
      const char *a = p + 1, *q = a;
      while (q < fim && *q && *q != '"') q += (*q == '\\' && q[1]) ? 2 : 1;
      if (prof == 1 && (size_t)(q - a) == n && !strncmp(a, chave, n)) {
        const char *v = q + 1;
        while (v < fim && (unsigned char)*v <= ' ') v++;
        if (v < fim && *v == ':') {
          for (v++; v < fim && (unsigned char)*v <= ' '; v++) ;
          *vi = v;
          if (*v == '{' || *v == '[') *vf = js_fim(v);
          else if (*v == '"') {
            const char *r = v + 1;
            while (r < fim && *r && *r != '"') r += (*r == '\\' && r[1]) ? 2 : 1;
            *vf = r < fim ? r + 1 : fim;
          } else {
            const char *r = v;
            while (r < fim && *r != ',' && *r != '}' && *r != ']') r++;
            *vf = r;
          }
          return *vf > *vi;
        }
      }
      if (q >= fim || !*q) break;
      p = q;
      continue;
    }
    if (*p == '{' || *p == '[') prof++;
    else if ((*p == '}' || *p == ']') && --prof == 0) break;
  }
  return 0;
}

// O objeto `chave` da raiz ("show", "movie", "episode"). 0 quando falta ou e
// null — all-items manda "next_to_watch": null e playback pode nao ter
// "episode".
static int objetoRaiz(const char *ini, const char *fim, const char *chave,
                      const char **oi, const char **of) {
  return valorRaiz(ini, fim, chave, oi, of) && **oi == '{';
}

// Titulo, ano e ids.imdb de um objeto de obra (show/movie/anime). 0 sem imdb.
static int lerObra(const char *oi, const char *of, CatItem *d, char *imdb,
                   size_t nImdb) {
  const char *ii, *ifim;
  imdb[0] = 0;
  js_texto_raiz_em(oi, of, "title", d->titulo, sizeof d->titulo);
  if (objetoRaiz(oi, of, "ids", &ii, &ifim))
    js_texto(ii, ifim, "imdb", imdb, nImdb);
  // Um id que nao e "tt..." nao serve a ninguem aqui: metahub, Cinemeta e os
  // addons indexam pelo IMDb. Recusar na leitura evita um card sem arte.
  if (imdb[0] != 't' || imdb[1] != 't') { imdb[0] = 0; return 0; }
  { int ano = (int)js_num(oi, of, "year", 0.0);
    if (ano > 1800) snprintf(d->meta, sizeof d->meta, "%d", ano); }
  return 1;
}

// ------------------------------------------------------------ leitores puros

int simkl_ler_playback(const char *json, CatItem *dst, long long *ids, int max) {
  const char *p;
  int n = 0;
  if (!json || !dst) return 0;
  for (p = js_raiz_array(json); p && *p == '{' && n < max; p = js_prox(js_fim(p))) {
    const char *f = js_fim(p), *oi, *of, *ei = NULL, *ef = NULL, *vi, *vf;
    CatItem *d = &dst[n];
    char imdb[24], quando[40] = "";
    int serie = 0, anime = 0;
    memset(d, 0, sizeof *d);
    if (objetoRaiz(p, f, "movie", &oi, &of)) serie = 0;
    else if (objetoRaiz(p, f, "show", &oi, &of)) serie = 1;
    else if (objetoRaiz(p, f, "anime", &oi, &of)) serie = anime = 1;
    else continue;
    if (!lerObra(oi, of, d, imdb, sizeof imdb)) continue;
    if (serie) {
      int t = 0, e = 0;
      if (!objetoRaiz(p, f, "episode", &ei, &ef)) continue;
      // ANIME SO COM A NUMERACAO DO TVDB. A doc: "Anime episodes return both
      // AniDB episode numbers (season, number) and TVDB numbers (tvdb_season,
      // tvdb_number)". O resto do app fala TVDB; o AniDB poria o card no
      // episodio errado — pior que nao mostrar.
      t = (int)js_num(ei, ef, "tvdb_season", 0.0);
      e = (int)js_num(ei, ef, "tvdb_number", 0.0);
      if (!anime && (t <= 0 || e <= 0)) {
        t = (int)js_num(ei, ef, "season", 0.0);
        e = (int)js_num(ei, ef, "episode", 0.0);
        if (e <= 0) e = (int)js_num(ei, ef, "number", 0.0);
      }
      if (t <= 0 || e <= 0) continue;
      d->temporada = t;
      d->episodio = e;
      js_texto_raiz_em(ei, ef, "title", d->nomeEpisodio, sizeof d->nomeEpisodio);
      snprintf(d->imdb, sizeof d->imdb, "%s:%d:%d", imdb, t, e);
      snprintf(d->tipo, sizeof d->tipo, "series");
    } else {
      snprintf(d->imdb, sizeof d->imdb, "%s", imdb);
      snprintf(d->tipo, sizeof d->tipo, "movie");
    }
    // `progress` e porcentagem em float (0-100, "45.5"). Os limites de 1% a
    // 90% sao aplicados em montarContinuar, igual para as tres fontes.
    if (valorRaiz(p, f, "progress", &vi, &vf)) d->progresso = (int)atof(vi);
    if (d->progresso < ajustes_cw_minimo() || d->progresso >= ajustes_cw_concluido()) continue;
    if (js_texto_raiz_em(p, f, "paused_at", quando, sizeof quando))
      d->retomadoMs = js_ms_iso(quando);
    if (ids) {
      // O "id" da RAIZ e o do registro de playback — o que DELETE
      // /sync/playback/{id} pede. "64bit integer" na doc: atof guarda inteiro
      // exato ate 2^53, e o id do Simkl esta longe disso.
      ids[n] = valorRaiz(p, f, "id", &vi, &vf) ? (long long)atof(vi) : 0;
    }
    n++;
  }
  return n;
}

int simkl_ler_assistindo(const char *json, CatItem *dst, int max) {
  const char *p;
  int n = 0;
  if (!json || !dst) return 0;
  // Sem nada em "watching" o Simkl responde `null` ou `{}` — js_array devolve
  // NULL e o laco nem comeca.
  for (p = js_array(json, NULL, "shows"); p && *p == '{' && n < max;
       p = js_prox(js_fim(p))) {
    const char *f = js_fim(p), *oi, *of;
    CatItem *d = &dst[n];
    char imdb[24], prox[16] = "", quando[40] = "";
    int t = 0, e = 0;
    memset(d, 0, sizeof *d);
    // "S01E05". Em dia com o que foi ao ar vem null, e ai nao ha o que seguir.
    if (!js_texto_raiz_em(p, f, "next_to_watch", prox, sizeof prox)) continue;
    if (sscanf(prox, "S%dE%d", &t, &e) != 2 || t <= 0 || e <= 0) continue;
    if (!objetoRaiz(p, f, "show", &oi, &of)) continue;
    if (!lerObra(oi, of, d, imdb, sizeof imdb)) continue;
    d->temporada = t;
    d->episodio = e;
    d->progresso = 0;
    snprintf(d->imdb, sizeof d->imdb, "%s:%d:%d", imdb, t, e);
    snprintf(d->tipo, sizeof d->tipo, "series");
    // O instante do "a seguir" e o do ULTIMO EPISODIO VISTO, como no Trakt
    // (ult[].quandoMs): e ele que diz quando a pessoa mexeu nesta serie.
    if (js_texto_raiz_em(p, f, "last_watched_at", quando, sizeof quando))
      d->retomadoMs = js_ms_iso(quando);
    n++;
  }
  return n;
}

int simkl_ler_plantowatch(const char *json, int serie, CatItem *dst, int max) {
  const char *p;
  int n = 0;
  if (!json || !dst) return 0;
  for (p = js_array(json, NULL, serie ? "shows" : "movies"); p && *p == '{' && n < max;
       p = js_prox(js_fim(p))) {
    const char *f = js_fim(p), *oi, *of;
    CatItem *d = &dst[n];
    char imdb[24];
    memset(d, 0, sizeof *d);
    if (!objetoRaiz(p, f, serie ? "show" : "movie", &oi, &of)) continue;
    if (!lerObra(oi, of, d, imdb, sizeof imdb)) continue;
    snprintf(d->imdb, sizeof d->imdb, "%s", imdb);
    snprintf(d->tipo, sizeof d->tipo, "%s", serie ? "series" : "movie");
    d->naLista = 1;
    // O mesmo acabamento de trakt_lista: arte deterministica pelo IMDb, sem
    // um GET por item, e o genero de sempre para a legenda do cartaz.
    arte_metahub_preencher(d);
    snprintf(d->genero, sizeof d->genero, "%s",
             i18n(serie ? "Programa de TV" : "Filme"));
    n++;
  }
  return n;
}

static int imdbValido(const char *s) {
  int i;
  if (!s || s[0] != 't' || s[1] != 't' || !s[2]) return 0;
  for (i = 2; s[i] && s[i] != ':'; i++) if (s[i] < '0' || s[i] > '9') return 0;
  return i < 20;
}

int simkl_corpo_lista(char *dst, size_t tam, const char *imdb,
                      const char *tipo, int adicionar) {
  char id[24];
  const char *dp;
  size_t k;
  int w;
  if (!dst || !tam || !imdbValido(imdb)) return 0;
  dp = strchr(imdb, ':');
  k = dp ? (size_t)(dp - imdb) : strlen(imdb);
  if (k >= sizeof id) return 0;
  memcpy(id, imdb, k); id[k] = 0;
  // Filme e serie em vetores separados, como no Trakt: o id do IMDb resolvido
  // no escopo errado casa outro titulo, ou nenhum. Anime vai em "shows" — a
  // doc do add-to-list e do history/remove aceita os dois ali.
  w = snprintf(dst, tam,
               adicionar ? "{\"%s\":[{\"to\":\"plantowatch\",\"ids\":{\"imdb\":\"%s\"}}]}"
                         : "{\"%s\":[{\"ids\":{\"imdb\":\"%s\"}}]}",
               (tipo && !strcmp(tipo, "series")) ? "shows" : "movies", id);
  return w > 0 && (size_t)w < tam;
}

const char *simkl_rota_lista(int adicionar) {
  return adicionar ? "/sync/add-to-list" : "/sync/history/remove";
}

// ------------------------------------------------------------ historico (visto)
//
// Corpos de /sync/history e /sync/history/remove. Ver simkl.h para as rotas e a
// fonte; aqui so a montagem, pura, para o teste conferir texto contra texto.

// Anexa com snprintf e diz se coube. `*u` avanca; estourou, o corpo inteiro e
// recusado (0) em vez de sair cortado — JSON pela metade o servidor le como
// 400, ou pior, como outro titulo.
static int anexar(char *dst, size_t tam, size_t *u, const char *fmt, int a, int b) {
  int w;
  if (*u >= tam) return 0;
  w = snprintf(dst + *u, tam - *u, fmt, a, b);
  if (w < 0 || (size_t)w >= tam - *u) return 0;
  *u += (size_t)w;
  return 1;
}

static int baseImdb(const char *imdb, char *id, size_t tam) {
  size_t k;
  if (!imdbValido(imdb)) return 0;
  k = strcspn(imdb, ":");
  if (k >= tam) return 0;
  memcpy(id, imdb, k); id[k] = 0;
  return 1;
}

int simkl_corpo_historico_eps(char *dst, size_t tam, const char *imdb,
                              const VistoPar *pares, int qtd) {
  char id[24], feita[SMK_LOTE_MAX];
  size_t u = 0;
  int i, j, prim = 1;
  if (!dst || !tam || !pares || qtd < 1 || !baseImdb(imdb, id, sizeof id)) return 0;
  if (qtd > SMK_LOTE_MAX) qtd = SMK_LOTE_MAX;
  memset(feita, 0, sizeof feita);
  { int w = snprintf(dst, tam, "{\"shows\":[{\"ids\":{\"imdb\":\"%s\"},\"seasons\":[", id);
    if (w < 0 || (size_t)w >= tam) return 0;
    u = (size_t)w; }
  // AGRUPA POR TEMPORADA sem assumir ordem, como trakt_episodios_marcar: o
  // lote "ate aqui" atravessa temporadas e cada uma tem de aparecer UMA vez.
  // Sem watched_at: a doc diz que o servidor usa a hora do pedido.
  for (i = 0; i < qtd; i++) {
    int primEp = 1;
    if (feita[i]) continue;
    if (!anexar(dst, tam, &u, prim ? "{\"number\":%d,\"episodes\":[" : ",{\"number\":%d,\"episodes\":[",
                pares[i].temporada, 0)) return 0;
    prim = 0;
    for (j = i; j < qtd; j++) {
      if (feita[j] || pares[j].temporada != pares[i].temporada) continue;
      feita[j] = 1;
      if (!anexar(dst, tam, &u, primEp ? "{\"number\":%d}" : ",{\"number\":%d}",
                  pares[j].episodio, 0)) return 0;
      primEp = 0;
    }
    if (!anexar(dst, tam, &u, "]}", 0, 0)) return 0;
  }
  return anexar(dst, tam, &u, "]}]}", 0, 0);
}

int simkl_corpo_historico_titulo(char *dst, size_t tam, const char *imdb,
                                 const char *tipo, const int *temporadas,
                                 int nt, int visto) {
  char id[24];
  size_t u = 0;
  int i, serie = tipo && !strcmp(tipo, "series");
  if (!dst || !tam || !baseImdb(imdb, id, sizeof id)) return 0;
  if (!serie || visto) {
    // Filme (os dois sentidos) e serie marcada: o objeto so com ids. Para
    // serie, a guia mark-as-watched: "Drop both seasons and episodes" marca
    // todo episodio.
    int w = snprintf(dst, tam, "{\"%s\":[{\"ids\":{\"imdb\":\"%s\"}}]}",
                     serie ? "shows" : "movies", id);
    return w > 0 && (size_t)w < tam;
  }
  // DESMARCAR SERIE SEM TEMPORADAS APAGA A SERIE DA BIBLIOTECA do Simkl (aviso
  // da pagina remove-from-history). Com a lista de temporadas, cada entrada sem
  // `episodes` e expandida pelo servidor e a serie fica na biblioteca. Sem
  // temporada conhecida nao ha corpo seguro: 0, e nada e mandado.
  if (!temporadas || nt < 1) return 0;
  { int w = snprintf(dst, tam, "{\"shows\":[{\"ids\":{\"imdb\":\"%s\"},\"seasons\":[", id);
    if (w < 0 || (size_t)w >= tam) return 0;
    u = (size_t)w; }
  for (i = 0; i < nt; i++)
    if (!anexar(dst, tam, &u, i ? ",{\"number\":%d}" : "{\"number\":%d}", temporadas[i], 0))
      return 0;
  return anexar(dst, tam, &u, "]}]}", 0, 0);
}

const char *simkl_rota_historico(int visto) {
  return visto ? "/sync/history" : "/sync/history/remove";
}

// ------------------------------------------------------------ transporte

// Um pedido a api. `consulta` vai DEPOIS dos tres parametros obrigatorios.
// `metodo`: 'G' GET, 'P' POST, 'D' DELETE. O token entra so no cabecalho, e
// nada aqui imprime a URL — o client_id nao e segredo, mas o costume do app e
// logar so o caminho (ver rede_url_publica).
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static uint64_t cacheGeracao = 1;
typedef SimklDestino SmkPedido;
static SmkPedido pedidoCapturar(void) {
  SmkPedido p = {0}; p.sessao = nv_perfil_sessao_ler();
  if (!nv_perfil_sessao_travar(p.sessao)) return p;
  const char *token = simklauth_token();
  if (token && *token && nuvem_simkl_cliente()[0]) {
    snprintf(p.aut, sizeof p.aut, "Authorization: Bearer %s", token);
    nuvem_url_escapar(nuvem_simkl_cliente(), p.cid, sizeof p.cid);
    nuvem_url_escapar(nuvem_simkl_app()[0] ? nuvem_simkl_app() : "nuvio", p.nome, sizeof p.nome);
  }
  pthread_mutex_lock(&trava); p.geracao = cacheGeracao; pthread_mutex_unlock(&trava);
  nv_perfil_sessao_destravar();
  return p;
}
// Profile opening first, private state second; neither lock crosses HTTP.
static int pedidoTravar(SmkPedido p) {
  if (!nv_perfil_sessao_travar(p.sessao)) return 0;
  pthread_mutex_lock(&trava);
  if (p.geracao != cacheGeracao) {
    pthread_mutex_unlock(&trava); nv_perfil_sessao_destravar(); return 0;
  }
  return 1;
}
static void pedidoDestravar(void) { pthread_mutex_unlock(&trava); nv_perfil_sessao_destravar(); }
static int pedidoAtual(SmkPedido p) {
  if (!pedidoTravar(p)) return 0;
  pedidoDestravar(); return 1;
}

static char *pedir(SmkPedido pedido, char metodo, const char *caminho, const char *consulta,
                   const char *corpo, int *st) {
  char url[640];
  const char *cab[3];
  char *r;
  *st = 0;
  if (!pedido.aut[0] || (metodo == 'G' && !pedidoAtual(pedido))) return NULL;
  snprintf(url, sizeof url, "%s%s?client_id=%s&app-name=%s&app-version=1.0.1%s%s",
           SMK_API, caminho, pedido.cid, pedido.nome, consulta ? "&" : "", consulta ? consulta : "");
  cab[0] = pedido.aut;
  cab[1] = "Accept: application/json";
  cab[2] = NULL;
  if (metodo == 'P')      r = rede_postar_st(url, 20, cab, corpo ? corpo : "", st);
  else if (metodo == 'D') r = rede_apagar(url, 20, cab, st);
  else                    r = rede_baixar_st(url, 25, cab, st);
  return r;
}

static char *pegar(SmkPedido pedido, const char *caminho, const char *consulta, int *st) {
  char *r = pedir(pedido, 'G', caminho, consulta, NULL, st);
  if (r && (*st < 200 || *st >= 300 || !pedidoAtual(pedido))) {
    printf("[simkl] %s -> HTTP %d\n", caminho, *st);
    fflush(stdout);
    free(r);
    return NULL;
  }
  return r;
}

// Assinatura barata de um corpo (FNV-1a). 0 = sem corpo.
static unsigned long assinar(const char *s) {
  unsigned long h = 1469598103UL;
  if (!s || !*s) return 0;
  for (; *s; s++) h = (h ^ (unsigned char)*s) * 16777619UL;
  return h ? h : 1;
}

// ------------------------------------------------------------ estado

static volatile int listaEstado;
static int listaFioVivo;
static uint64_t listaSessao;

// Registros de playback da ultima leitura (chave composta -> id), para o
// DELETE, e os ids "tt:S:E" que sao "a seguir". Tabelas laterais pela mesma
// razao de trakt.c: sizeof(CatItem) e o cabecalho do cache em disco.
static struct { char chave[28]; long long id; } play[SMK_PLAY_MAX];
static int nPlay;
static char proxIds[SMK_PLAY_MAX][28];
static int nProx;
// Plan to Watch conhecido: o que a ultima leitura trouxe e o que o "+" daqui
// pos depois dela. E so a guarda do "-" (ver simkl.h).
static char ptw[SMK_PTW_MAX][24];
static int nPtw;

// CACHE PELO /sync/activities. A doc e explicita: "For continuous sync, do NOT
// call /sync/all-items on a timer", e ameaca suspender o client_id. A regra
// aqui e a mais simples que a cumpre: o corpo das atividades e guardado por
// assinatura; igual ao da ultima leitura = nada mudou em lista nenhuma, e os
// corpos guardados valem. As atividades custam um GET pequeno por ciclo, que e
// o que a doc chama de "the cheapest call in the API". Nao usa date_from: as
// listas lidas aqui (watching, plantowatch) sao pequenas, e o incremental
// exigiria guardar e fundir o estado entre arranques.
static unsigned long assCw, assPtw;
static char *corpoPlay, *corpoAssist, *corpoPtwFilmes, *corpoPtwSeries;

static void trocar(char **dst, char *novo) { free(*dst); *dst = novo; }

void simkl_esquecer(void) {
  pthread_mutex_lock(&trava);
  if (!++cacheGeracao) ++cacheGeracao;
  listaFioVivo = 0;
  __atomic_store_n(&listaEstado, SMK_OP_NENHUMA, __ATOMIC_RELEASE);
  nPlay = nProx = nPtw = 0;
  assCw = assPtw = 0;
  trocar(&corpoPlay, NULL); trocar(&corpoAssist, NULL);
  trocar(&corpoPtwFilmes, NULL); trocar(&corpoPtwSeries, NULL);
  pthread_mutex_unlock(&trava);
}

void simkl_abrir_sessao(void) { simkl_esquecer(); }

static unsigned long atividades(SmkPedido pedido) {
  int st = 0;
  char *r = pegar(pedido, "/sync/activities", NULL, &st);
  unsigned long h = assinar(r);
  free(r);
  return h;
}

int simkl_e_a_seguir(const char *id) {
  int i, sim = 0;
  if (!id) return 0;
  pthread_mutex_lock(&trava);
  for (i = 0; i < nProx && !sim; i++) sim = !strcmp(proxIds[i], id);
  pthread_mutex_unlock(&trava);
  return sim;
}

static int mesmaSerie(const char *a, const char *b) {
  size_t la = strcspn(a, ":"), lb = strcspn(b, ":");
  return la == lb && !strncmp(a, b, la);
}

int simkl_continuar(CatItem *saida, int max) {
  CatItem *prox;
  long long ids[SMK_PLAY_MAX];
  unsigned long h;
  char *cPlay = NULL, *cAssist = NULL;
  char proxLocal[SMK_PLAY_MAX][28]; int nProxLocal = 0;
  int n, nP, i, k;
  SmkPedido pedido = pedidoCapturar();
  if (!saida || max <= 0 || !pedido.aut[0]) return 0;
  if (max > SMK_PLAY_MAX) max = SMK_PLAY_MAX;
  h = atividades(pedido);
  if (!pedidoTravar(pedido)) return 0;
  if (h && h == assCw && corpoPlay && corpoAssist) {
    cPlay = strdup(corpoPlay); cAssist = strdup(corpoAssist);
  }
  pedidoDestravar();
  if (!cPlay || !cAssist) {
    int st = 0; free(cPlay); free(cAssist);
    cPlay = pegar(pedido, "/sync/playback", NULL, &st);
    cAssist = pegar(pedido, "/sync/all-items/shows/watching", NULL, &st);
    if (pedidoTravar(pedido)) {
      if (h && cPlay && cAssist) {
        trocar(&corpoPlay, strdup(cPlay)); trocar(&corpoAssist, strdup(cAssist)); assCw = h;
      } else assCw = 0;
      pedidoDestravar();
    }
  }
  if (!pedidoAtual(pedido)) { free(cPlay); free(cAssist); return 0; }
  n = simkl_ler_playback(cPlay, saida, ids, max);
  int nPausados = n;
  prox = malloc(sizeof(CatItem) * SMK_PLAY_MAX);
  nP = prox && ajustes_cw_proximo() ? simkl_ler_assistindo(cAssist, prox, SMK_PLAY_MAX) : 0;
  for (k = 0; k < nP; k++) {
    int ja = 0, alvo;
    for (i = 0; i < n && !ja; i++) ja = mesmaSerie(saida[i].imdb, prox[k].imdb);
    if (ja) continue;
    if (n < max) alvo = n++;
    else {
      int vel = -1;
      for (i = 0; i < n; i++)
        if (saida[i].progresso == 0 &&
            (vel < 0 || saida[i].retomadoMs < saida[vel].retomadoMs)) vel = i;
      if (vel < 0 || saida[vel].retomadoMs >= prox[k].retomadoMs) continue;
      alvo = vel;
    }
    saida[alvo] = prox[k];
    if (nProxLocal < SMK_PLAY_MAX)
      snprintf(proxLocal[nProxLocal++], sizeof proxLocal[0], "%s", prox[k].imdb);
  }
  free(prox); free(cPlay); free(cAssist);
  if (!pedidoTravar(pedido)) return 0;
  nPlay = 0;
  for (i = 0; i < nPausados; i++)
    if (ids[i] > 0 && nPlay < SMK_PLAY_MAX) {
      snprintf(play[nPlay].chave, sizeof play[nPlay].chave, "%s", saida[i].imdb);
      play[nPlay++].id = ids[i];
    }
  nProx = nProxLocal;
  memcpy(proxIds, proxLocal, sizeof proxIds[0] * (size_t)nProx);
  pedidoDestravar();
  { int a, b;
    for (a = 1; a < n; a++) {
      CatItem t = saida[a];
      for (b = a - 1; b >= 0 &&
           ((saida[b].progresso == 0 && t.progresso > 0) ||
            ((saida[b].progresso == 0) == (t.progresso == 0) &&
             saida[b].retomadoMs < t.retomadoMs)); b--) saida[b + 1] = saida[b];
      saida[b + 1] = t;
    }
  }
  printf("[simkl] retomada: %d pausado(s) + %d a seguir candidato(s) -> %d\n", nPausados, nP, n);
  fflush(stdout);
  if (n) n = trakt_enfeitar_lote_sessao ? trakt_enfeitar_lote_sessao(saida, n, pedido.sessao)
                                       : trakt_enfeitar_lote(saida, n);
  return pedidoAtual(pedido) ? n : 0;
}

int simkl_plantowatch(CatItem *saida, int max) {
  unsigned long h; char *cF = NULL, *cS = NULL; int n = 0, i;
  SmkPedido pedido = pedidoCapturar();
  if (!saida || max <= 0 || !pedido.aut[0]) return 0;
  h = atividades(pedido);
  if (!pedidoTravar(pedido)) return 0;
  if (h && h == assPtw && corpoPtwFilmes && corpoPtwSeries) {
    cF = strdup(corpoPtwFilmes); cS = strdup(corpoPtwSeries);
  }
  pedidoDestravar();
  if (!cF || !cS) {
    int st = 0; free(cF); free(cS);
    cF = pegar(pedido, "/sync/all-items/movies/plantowatch", NULL, &st);
    cS = pegar(pedido, "/sync/all-items/shows/plantowatch", NULL, &st);
    if (pedidoTravar(pedido)) {
      if (h && cF && cS) {
        trocar(&corpoPtwFilmes, strdup(cF)); trocar(&corpoPtwSeries, strdup(cS)); assPtw = h;
      } else assPtw = 0;
      pedidoDestravar();
    }
  }
  n = simkl_ler_plantowatch(cF, 0, saida, max);
  n += simkl_ler_plantowatch(cS, 1, saida + n, max - n);
  if (cF && cS && pedidoTravar(pedido)) {
    nPtw = 0;
    for (i = 0; i < n && nPtw < SMK_PTW_MAX; i++)
      snprintf(ptw[nPtw++], sizeof ptw[0], "%s", saida[i].imdb);
    pedidoDestravar();
  }
  free(cF); free(cS);
  printf("[simkl] plantowatch: %d\n", n); fflush(stdout);
  return pedidoAtual(pedido) ? n : 0;
}

int simkl_na_plantowatch(const char *imdb) {
  int i, sim = 0;
  size_t L;
  if (!imdb || !imdb[0]) return 0;
  L = strcspn(imdb, ":");
  pthread_mutex_lock(&trava);
  for (i = 0; i < nPtw && !sim; i++)
    sim = strlen(ptw[i]) == L && !strncmp(ptw[i], imdb, L);
  pthread_mutex_unlock(&trava);
  return sim;
}

// ------------------------------------------------------------ escritas

typedef struct { char id[24], tipo[8]; int adicionar; SmkPedido pedido; } SmkLista;
static void ptwMarcar(const char *id, int dentro) { // caller owns pedidoTravar
  int i;
  for (i = 0; i < nPtw; i++) if (!strcmp(ptw[i], id)) break;
  if (dentro && i == nPtw && nPtw < SMK_PTW_MAX) snprintf(ptw[nPtw++], sizeof ptw[0], "%s", id);
  if (!dentro && i < nPtw) { memmove(ptw[i], ptw[i + 1], sizeof ptw[0] * (size_t)(nPtw - i - 1)); nPtw--; }
  assPtw = 0;
}
static void *fioLista(void *u) {
  SmkLista *a = u; char corpo[200], *r; int st = 0, ok;
  ok = simkl_corpo_lista(corpo, sizeof corpo, a->id, a->tipo, a->adicionar);
  r = ok ? pedir(a->pedido, 'P', simkl_rota_lista(a->adicionar), NULL, corpo, &st) : NULL;
  ok = st >= 200 && st < 300;
  if (pedidoTravar(a->pedido)) {
    if (ok) ptwMarcar(a->id, a->adicionar);
    __atomic_store_n(&listaEstado, ok ? SMK_OP_CONFIRMADA : SMK_OP_FALHA, __ATOMIC_RELEASE);
    listaFioVivo = 0;
    pedidoDestravar();
  }
  printf("[simkl] plantowatch %s %s (%s) -> HTTP %d\n", a->adicionar ? "add" : "del", a->id, a->tipo, st);
  fflush(stdout); free(r); memset(a, 0, sizeof *a); free(a);
  return NULL;
}
int simkl_lista_tipo(const char *imdb, const char *tipo, int adicionar) {
  pthread_t t;
  if (!imdbValido(imdb)) { __atomic_store_n(&listaEstado, SMK_OP_FALHA, __ATOMIC_RELEASE); return 0; }
  SmkLista *a = calloc(1, sizeof *a); if (!a) return 0;
  a->pedido = pedidoCapturar();
  if (!a->pedido.aut[0]) {
    if (pedidoTravar(a->pedido)) { __atomic_store_n(&listaEstado, SMK_OP_FALHA, __ATOMIC_RELEASE); pedidoDestravar(); }
    free(a); return 0;
  }
  if (!pedidoTravar(a->pedido)) { free(a); return 0; }
  int dentro = 0; size_t k = strcspn(imdb, ":");
  for (int i = 0; i < nPtw && !dentro; i++) dentro = strlen(ptw[i]) == k && !strncmp(ptw[i], imdb, k);
  if ((!adicionar && !dentro) || (listaFioVivo && listaSessao == a->pedido.sessao)) {
    if (!adicionar && !dentro) __atomic_store_n(&listaEstado, SMK_OP_FALHA, __ATOMIC_RELEASE);
    pedidoDestravar(); free(a); return 0;
  }
  if (k >= sizeof a->id) k = sizeof a->id - 1;
  memcpy(a->id, imdb, k); a->id[k] = 0;
  snprintf(a->tipo, sizeof a->tipo, "%s",
           (tipo && !strcmp(tipo, "series")) || strchr(imdb, ':') ? "series" : "movie");
  a->adicionar = adicionar;
  listaFioVivo = 1; listaSessao = a->pedido.sessao;
  __atomic_store_n(&listaEstado, SMK_OP_PENDENTE, __ATOMIC_RELEASE);
  int ok = pthread_create(&t, NULL, fioLista, a) == 0;
  if (ok) pthread_detach(t);
  else { listaFioVivo = 0; __atomic_store_n(&listaEstado, SMK_OP_FALHA, __ATOMIC_RELEASE); free(a); }
  pedidoDestravar(); return ok;
}
int simkl_lista_estado(void) { return __atomic_load_n(&listaEstado, __ATOMIC_ACQUIRE); }

// POST do historico, SINCRONO (quem chama ja esta num fio: visto.c). 2xx
// sozinho nao prova nada: a doc manda olhar `not_found`, que volta com os
// objetos que o servidor nao resolveu. Um `"ids"` dentro dele e titulo que o
// Simkl nao achou pelo imdb — conta como falha, e o log diz.
// `qtd` > 0: lote de episodios; 0: titulo inteiro (`serie` diz qual).
static int postarHistorico(SmkPedido pedido, const char *corpo, int visto, const char *id, int qtd, int serie) {
  char *r;
  int st = 0, ok;
  const char *nf;
  r = pedir(pedido, 'P', simkl_rota_historico(visto), NULL, corpo, &st);
  ok = st >= 200 && st < 300;
  nf = r ? strstr(r, "\"not_found\"") : NULL;
  if (ok && nf && strstr(nf, "\"ids\"")) ok = 0;
  if (qtd > 0) printf("[simkl] historico %s %d eps de %s", visto ? "add" : "del", qtd, id);
  else printf("[simkl] historico %s %s %s", visto ? "add" : "del", serie ? "serie" : "filme", id);
  printf(" -> %s (HTTP %d)%s\n", ok ? "ok" : "falhou", st,
         nf && strstr(nf, "\"ids\"") ? " [nao encontrado no Simkl]" : "");
  fflush(stdout);
  free(r);
  return ok;
}

int simkl_episodios_marcar_destino(const SimklDestino *destino, const char *imdb, const VistoPar *pares, int qtd, int visto) {
  char corpo[SMK_LOTE_MAX * 20 + 200];
  int ok;
  if (!destino || !destino->aut[0] || !imdb) return 0;
  SmkPedido pedido = *destino;
  if (!simkl_corpo_historico_eps(corpo, sizeof corpo, imdb, pares, qtd)) return 0;
  ok = postarHistorico(pedido, corpo, visto, imdb, qtd > SMK_LOTE_MAX ? SMK_LOTE_MAX : qtd, 1);
  return ok;
}

int simkl_titulo_marcar_destino(const SimklDestino *destino, const char *imdb, const char *tipo, const int *temporadas,
                        int nt, int visto) {
  char corpo[1200];
  if (!destino || !destino->aut[0] || !imdb) return 0;
  SmkPedido pedido = *destino;
  if (!simkl_corpo_historico_titulo(corpo, sizeof corpo, imdb, tipo, temporadas, nt, visto)) {
    printf("[simkl] historico %s %s: sem corpo seguro (serie sem temporadas?); nada mandado\n",
           visto ? "add" : "del", imdb);
    fflush(stdout);
    return 0;
  }
  return postarHistorico(pedido, corpo, visto, imdb, 0, tipo && !strcmp(tipo, "series"));
}

int simkl_destino_capturar(SimklDestino *d) {
  if (!d) return 0;
  *d = pedidoCapturar();
  return d->aut[0] != 0;
}
int simkl_episodios_marcar(const char *imdb, const VistoPar *pares, int qtd, int visto) {
  SimklDestino d;
  return simkl_destino_capturar(&d) && simkl_episodios_marcar_destino(&d, imdb, pares, qtd, visto);
}
int simkl_titulo_marcar(const char *imdb, const char *tipo, const int *temporadas, int nt, int visto) {
  SimklDestino d;
  return simkl_destino_capturar(&d) && simkl_titulo_marcar_destino(&d, imdb, tipo, temporadas, nt, visto);
}

typedef struct { long long id; SmkPedido pedido; } SmkApagar;
static void *fioApagar(void *u) {
  SmkApagar *a = u; char caminho[64], *r; int st = 0, ok;
  snprintf(caminho, sizeof caminho, "/sync/playback/%lld", a->id);
  r = pedir(a->pedido, 'D', caminho, NULL, NULL, &st);
  ok = st >= 200 && st < 300; free(r);
  if (ok && pedidoTravar(a->pedido)) {
    for (int i = 0; i < nPlay; i++) if (play[i].id == a->id) { play[i].chave[0] = 0; break; }
    assCw = 0; pedidoDestravar();
  }
  printf("[simkl] playback remover id %lld -> HTTP %d\n", a->id, st); fflush(stdout);
  memset(a, 0, sizeof *a); free(a); return NULL;
}
int simkl_playback_remover(const char *imdb) {
  pthread_t t;
  if (!imdb || !imdb[0]) return 0;
  SmkApagar *a = calloc(1, sizeof *a); if (!a) return 0;
  a->pedido = pedidoCapturar();
  if (!a->pedido.aut[0] || !pedidoTravar(a->pedido)) { free(a); return 0; }
  for (int i = 0; i < nPlay; i++) if (!strcmp(play[i].chave, imdb)) { a->id = play[i].id; break; }
  pedidoDestravar();
  if (!a->id || pthread_create(&t, NULL, fioApagar, a) != 0) { free(a); return 0; }
  pthread_detach(t); return 1;
}
