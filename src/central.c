// Ver central.h.
#define NV_ESCALA_TELA
#include "central.h"
#include "centrallista.h"
#include "detail.h"
#include "chsegura.h"
#include "ajustes.h"
#include "anim.h"
#include "avatarperfil.h"
#include "catalogo.h"
#include "dados.h"
#include "escala.h"
#include "estatvivo.h"
#include "estatvivoui.h"
#include "gfx.h"
#include "horafmt.h"
#include "idioma.h"
#include "idiomacod.h"
#include "layout.h"
#include "perfiltv.h"
#include "perfis.h"
#include "player.h"
#include "ponteiro.h"
#include "ilha.h"
#include "plrilha.h"
#include "redesaude.h"
#include "tex_cache.h"
#include "text.h"
#include "video.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef NV_VERSAO
#define NV_VERSAO "dev"
#endif

// O PAINEL e a ilha do relogio esticada, numa coluna estreita pendurada no
// canto dela (dono, 06/10: "muito grandao... menos largo e so aumentar se
// tiver mais botoes"). A altura segue o conteudo: poucos botoes, painel curto.
#define CC_W       492.0f
#define CC_PAD      20.0f
#define CC_COLS      3
#define CC_GAP      10.0f
#define CC_BOTAO_H 104.0f
#define CC_BOTAO_R  22.0f
#define CC_ICONE    26.0f
#define CC_INFO_H   NV_ILHA_H   // a faixa de cima: a linha de informacao e a hora
#define CC_TOC_H    62.0f       // "Tocando agora", uma linha
#define CC_LINHA_H  50.0f       // linha da lista de edicao
#define CC_LINHA_GAP 4.0f
#define CC_LINHAS_MAX 9         // a lista rola dentro disso
#define CC_BOT_VIS    4         // fileiras de botoes visiveis; mais rolam (o foco segue)
#define CC_DICAS_H  30.0f
#define CC_AV_INFO  26.0f       // o avatar da linha de informacao
#define CC_COR_D    40.0f       // o disco de uma cor no seletor
#define CC_COR_COLS  9
#define CC_COR_LIN_GAP 14.0f
#define CC_COR_NOME_H 26.0f
#define CC_TETO     (NV_TELA_H - NV_ILHA_Y - 36.0f)

static int aberta, editando;
static int escolhendoCor, focoCor;   // o seletor da cor de destaque (so os 18 fixos)
static int foco;                // botao em foco; == nBotoes e "Editar atalhos"
static int focoEd;              // item do catalogo em foco na edicao
static void topoBotAjusta(void);
static int   topoBot;           // primeira fileira de botoes visivel (alvo)
static float rolaBot, rolaBotVel; // a mesma, suavizada, e a velocidade da mola
static float rolaEd;            // rolagem da lista de edicao (px)
static CentralLista lista;
static int perfilLido = -1;

// --- A TROCA RAPIDA DE PERFIL (2.0.4) -----------------------------------------
// O perfil da linha de cima e um botao: OK abre, dentro do painel, uma fileira
// de avatares com o nome; esquerda/direita andam, OK troca. Perfil com PIN pede
// o PIN ali mesmo (4 digitos, verificado no servidor por perfis_verificar_pin,
// como na tela de escolha). Quem troca de verdade e app.c, pelo MESMO corpo da
// tela de escolha (central_pediu_perfil). Com video no player a troca nao
// existe: ela esvaziaria a reproducao, entao a linha fica so informativa.
#define CC_PERF_CEL_W   113.0f     // uma celula = a largura util / 4
#define CC_PERF_VIS       4        // celulas visiveis; mais perfis rolam
#define CC_PERF_AV       64.0f
#define CC_PERF_CEL_H   118.0f
#define CC_PIN_LEN        4
#define CC_PIN_COLS       5
#define CC_PIN_TECLA_W   76.0f
#define CC_PIN_TECLA_H   46.0f
#define CC_PIN_GAP        8.0f
#define CC_PIN_CAB_H     44.0f     // avatar + nome de quem esta sendo destravado
#define CC_PIN_PONTOS_H  30.0f
#define CC_PIN_H  (CC_PIN_CAB_H + 10.0f + CC_PIN_PONTOS_H + 14.0f + 2.0f * CC_PIN_TECLA_H + CC_PIN_GAP)
static int focoInfo;            // o foco esta no perfil da linha de cima
static int trocando;            // o seletor de perfil aberto
static int focoPerf;            // slot (0..n-1 da lista) em foco
static float rolaPerf, rolaVel; // primeiro slot visivel (suavizado) e a velocidade da mola
static float placaPerf;         // onde a placa de foco esta, em slots (anda ate o foco)
static int pinSlot = -1;        // slot cujo PIN esta sendo pedido; -1 nenhum
static char pinBuf[CC_PIN_LEN + 1];
static int pinTecla;            // 0..9 na grade 5x2 (1..5 em cima, 6..9,0 embaixo)
static int pinErrado, pinRede, pinVerif;
static float pinTremor;         // s que faltam do balanco do PIN errado
static _Atomic int pinResultado;     // 0 pendente, 1 ok, -1 errado, -2 sem rede
static _Atomic unsigned pinGeracao;
static int pedidoPerfil;        // profile_index pedido, lido e zerado por app.c
typedef struct { unsigned geracao; int indice; char valor[CC_PIN_LEN + 1]; } PinTarefa;
#ifdef CENTRAL_TESTE
static int testePodeTrocar;
#endif
static int previa;   // definida mais abaixo, com previaInverte
static int podeTrocar(void) {
#ifdef CENTRAL_TESTE
  if (testePodeTrocar) return !detail_aberto() && perfis_n() > 1;
#endif
  // Nem com o player, nem com a pagina do titulo aberta: a troca esvaziaria o
  // que esta na tela. A linha do perfil fica so informativa.
  return !previa && !player_aberto() && !detail_aberto() && perfis_n() > 1;
}
int central_troca_disponivel(void) { return podeTrocar(); }
// A previa do cartao de novidades (central_previa_*): sem video tocando,
// sem aviso, e o botao `previaInverte` mostrado com o interruptor trocado.
static int previa, previaInverte = -1;
static char aviso[96];
static Uint32 avisoAte;

static ChSegura chs;
static SDL_Event chDown;        // o KEYDOWN guardado para reentregar
static int chLogado;

// --- A LISTA DO PERFIL -------------------------------------------------------
static void nomeArquivo(char *d, size_t n) { snprintf(d, n, "central-p%d.txt", perfis_ativo()); }
static void carregar(void) {
  char nome[40], *t;
  if (perfis_ativo() == perfilLido) return;
  perfilLido = perfis_ativo();
  nomeArquivo(nome, sizeof nome);
  t = dados_ler(nome);
  centrallista_ler(&lista, t);
  free(t);
}
static void gravar(void) {
  char nome[40], buf[CENTRAL_MAX * 48 + 8];
  nomeArquivo(nome, sizeof nome);
  centrallista_escrever(&lista, buf, sizeof buf);
  dados_gravar(nome, buf);
}

// ESTATISTICAS (mockup stats-velocidade, tela 1): com video no player, o
// primeiro botao e sempre o das estatisticas ao vivo (estatvivoui.h). Nao e
// uma opcao de Ajustes nem mora na lista da pessoa: e uma acao, e so existe
// enquanto ha o que medir. op = CC_OP_ESTAT, item = -1.
#define CC_OP_ESTAT (-2)
#ifdef CENTRAL_TESTE
static int testeEstat;
#endif
static int temEstat(void) {
#ifdef CENTRAL_TESTE
  if (testeEstat) return !previa;
#endif
  return !previa && player_aberto() && player_com_video();
}

// Os botoes que existem neste build (a chave pode nao estar na tela daqui).
static int botoes(int *op, int *item) {
  int i, n = 0;
  if (temEstat()) {
    if (op) op[n] = CC_OP_ESTAT;
    if (item) item[n] = -1;
    n++;
  }
  for (i = 0; i < lista.n; i++) {
    const CentralItem *c = central_catalogo(lista.item[i]);
    int o = c ? ajustes_rapido_op(c->chave) : -1;
    if (o < 0) continue;
    if (op) op[n] = o;
    if (item) item[n] = lista.item[i];
    n++;
  }
  return n;
}
// O catalogo oferecido na edicao: so o que este build tem.
static int ofertas(int *item) {
  int i, n = 0;
  for (i = 0; i < central_catalogo_n(); i++)
    if (ajustes_rapido_op(central_catalogo(i)->chave) >= 0) item[n++] = i;
  return n;
}

void central_abrir(void) {
  carregar();
  if (aberta) return;
  aberta = 1;
  editando = 0;
  escolhendoCor = 0;
  focoInfo = 0;
  trocando = 0;
  pinSlot = -1;
  foco = 0;
  topoBot = 0; rolaBot = 0.0f; rolaBotVel = 0.0f;
  aviso[0] = 0;
  printf("[central] aberta (%d atalhos)\n", botoes(NULL, NULL));
  fflush(stdout);
}
// O PIN digitado sai da memoria com a central, e uma verificacao em voo perde o
// direito de responder (a geracao muda).
static void pinLimpar(void) {
  atomic_fetch_add(&pinGeracao, 1);
  atomic_store(&pinResultado, 0);
  memset(pinBuf, 0, sizeof pinBuf);
  pinSlot = -1; pinVerif = pinErrado = pinRede = 0; pinTremor = 0.0f;
}
void central_fechar(void) {
  aberta = 0; editando = 0; escolhendoCor = 0;
  focoInfo = 0; trocando = 0;
  pinLimpar();
}
// Lido e zerado por app.c, uma vez por quadro: o profile_index que a pessoa
// escolheu na troca rapida, ou 0.
int central_pediu_perfil(void) { int v = pedidoPerfil; pedidoPerfil = 0; return v; }
int  central_aberta(void) { return aberta; }

static void avisar(const char *s) {
  snprintf(aviso, sizeof aviso, "%s", s);
  avisoAte = SDL_GetTicks() + 2500u;
}

// --- TECLADO -----------------------------------------------------------------
static int ehVoltar(SDL_Keycode k) {
  return k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE;
}
// O CH+ depois do remapeamento (Salvos na Samsung/Android, scancode no LG) e a
// AZUL: a mesma tecla que abriu fecha.
static int ehCh(const SDL_Event *e) {
  SDL_Keycode k = e->key.keysym.sym;
  int sc = e->key.keysym.scancode;
  return k == SDLK_s || k == SDLK_PAGEUP || sc == NV_SCANCODE_BLUE || sc == NV_SCANCODE_CH_UP;
}

static void ativarBotao(int i) {
  int op[CENTRAL_MAX + 1], n = botoes(op, NULL);
  if (i == n) {
    int it[64], m = ofertas(it), j;
    editando = 1; focoEd = 0; rolaEd = 0;
    for (j = 0; j < m; j++) if (centrallista_tem(&lista, it[j])) { focoEd = j; break; }
    return;
  }
  if (i < 0 || i >= n) return;
  if (op[i] == AJ_RAPIDO_COR) {
    int a = ajustes_rapido_cor_atual();
    escolhendoCor = 1;
    focoCor = a >= 0 ? a : 0;
    return;
  }
  if (op[i] == CC_OP_ESTAT) {
    central_fechar();
    estatvivo_abrir();
    return;
  }
  if (ajustes_rapido_passo(op[i], 1))
    printf("[central] %s -> %s\n", ajustes_rapido_rotulo(op[i]), ajustes_rapido_valor(op[i]));
  else avisar("Não foi possível salvar. O valor anterior foi mantido.");
  fflush(stdout);
}

static void alternarOferta(int j) {
  int it[64], m = ofertas(it), r;
  if (j < 0 || j >= m) return;
  r = centrallista_alternar(&lista, it[j]);
  if (r < 0) {
    char t[96];
    snprintf(t, sizeof t, i18n("No máximo %d atalhos"), CENTRAL_MAX);
    avisar(t);
    return;
  }
  gravar();
}

// Aplica a cor e grava como Ajustes; o painel inteiro ja se pinta com ela no
// proximo quadro (os botoes ligados e a marca leem ajustes_acento).
static void corAplicar(int i) {
  if (ajustes_rapido_cor_definir(i)) {
    printf("[central] %s -> %s\n", ajustes_rapido_rotulo(AJ_RAPIDO_COR), ajustes_rapido_cor_nome(i));
  } else avisar("Não foi possível salvar. O valor anterior foi mantido.");
  fflush(stdout);
}

// --- O SELETOR DE PERFIL ------------------------------------------------------
static float perfAlvoRola(void) {
  int n = perfis_n();
  float a = (float)focoPerf - 1.0f;
  if (n <= CC_PERF_VIS) return 0.0f;
  if (a < 0.0f) a = 0.0f;
  if (a > (float)(n - CC_PERF_VIS)) a = (float)(n - CC_PERF_VIS);
  return a;
}
static void abrirTrocador(void) {
  if (!podeTrocar()) return;
  trocando = 1; focoInfo = 0;
  pinLimpar();
  focoPerf = perfis_indice_sugerido();
  rolaPerf = perfAlvoRola(); rolaVel = 0.0f;
  placaPerf = (float)focoPerf;
}
static void *fioPinVerificar(void *u) {
  PinTarefa *t = u;
  int v = perfis_verificar_pin(t->indice, t->valor);
  memset(t->valor, 0, sizeof t->valor);   // o PIN nao fica no heap nem vai a log
  if (t->geracao == atomic_load(&pinGeracao))
    atomic_store(&pinResultado, v > 0 ? 1 : (v < 0 ? -2 : -1));
  free(t);
  return NULL;
}
static void pinVerificar(void) {
  PinTarefa *t;
  const ContaPerfil *p = perfis_item(pinSlot);
  pthread_t th;
  if (!p || !pinBuf[0]) return;
  t = calloc(1, sizeof *t);
  if (!t) { pinRede = 1; return; }
  t->geracao = atomic_load(&pinGeracao);
  t->indice = p->indice;
  snprintf(t->valor, sizeof t->valor, "%s", pinBuf);
  pinVerif = 1; pinErrado = pinRede = 0;
  atomic_store(&pinResultado, 0);
  if (pthread_create(&th, NULL, fioPinVerificar, t) == 0) pthread_detach(th);
  else { memset(t->valor, 0, sizeof t->valor); free(t); pinVerif = 0; pinRede = 1; memset(pinBuf, 0, sizeof pinBuf); }
}
static void pinDigito(int d) {
  size_t z = strlen(pinBuf);
  if (pinVerif || z >= CC_PIN_LEN) return;
  pinBuf[z] = (char)('0' + d); pinBuf[z + 1] = 0;
  pinErrado = pinRede = 0;
  if (z + 1 == CC_PIN_LEN) pinVerificar();
}
// Escolher o slot: o mesmo perfil so fecha o seletor; PIN pede o PIN; o resto
// vira o pedido que app.c executa.
static void escolherPerfil(int slot) {
  const ContaPerfil *p = perfis_item(slot);
  if (!p) return;
  if (p->indice == perfis_ativo()) { trocando = 0; return; }
  if (perfis_acao(slot) == PERFIL_ACAO_PIN) {
    pinLimpar();
    pinSlot = slot; pinTecla = 0;
    return;
  }
  pedidoPerfil = p->indice;
  printf("[central] troca rapida para o perfil %d\n", p->indice);
  fflush(stdout);
  central_fechar();
}
static void trocadorEvento(SDL_Keycode k) {
  int n = perfis_n();
  if (pinSlot >= 0) {
    if (ehVoltar(k)) {
      if (pinVerif) { pinLimpar(); }                       // desiste da verificacao
      else if (pinBuf[0]) { pinBuf[strlen(pinBuf) - 1] = 0; pinErrado = pinRede = 0; }
      else pinLimpar();                                    // volta para a fileira
      return;
    }
    if (pinVerif) return;
    if (k >= SDLK_0 && k <= SDLK_9) { pinDigito((int)(k - SDLK_0)); return; }
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) { pinDigito((int)(k - SDLK_KP_1) + 1); return; }
    if (k == SDLK_KP_0) { pinDigito(0); return; }
    if (k == SDLK_BACKSPACE) { if (pinBuf[0]) pinBuf[strlen(pinBuf) - 1] = 0; return; }
    if (k == SDLK_LEFT && pinTecla % CC_PIN_COLS > 0) pinTecla--;
    else if (k == SDLK_RIGHT && pinTecla % CC_PIN_COLS < CC_PIN_COLS - 1) pinTecla++;
    else if (k == SDLK_UP && pinTecla >= CC_PIN_COLS) pinTecla -= CC_PIN_COLS;
    else if (k == SDLK_DOWN && pinTecla < CC_PIN_COLS) pinTecla += CC_PIN_COLS;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) pinDigito(pinTecla == 9 ? 0 : pinTecla + 1);
    return;
  }
  if (ehVoltar(k)) { trocando = 0; return; }
  if (k == SDLK_LEFT && focoPerf > 0) focoPerf--;
  else if (k == SDLK_RIGHT && focoPerf + 1 < n) focoPerf++;
  else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) escolherPerfil(focoPerf);
}

void central_evento(const SDL_Event *e) {
  SDL_Keycode k;
  if (!aberta || e->type != SDL_KEYDOWN) return;
  k = e->key.keysym.sym;
  if (ehCh(e)) { central_fechar(); return; }
  if (escolhendoCor) {
    int n = ajustes_rapido_cor_n();
    if (ehVoltar(k)) { escolhendoCor = 0; return; }
    if (k == SDLK_LEFT && focoCor % CC_COR_COLS > 0) focoCor--;
    else if (k == SDLK_RIGHT && focoCor % CC_COR_COLS < CC_COR_COLS - 1 && focoCor + 1 < n) focoCor++;
    else if (k == SDLK_UP && focoCor >= CC_COR_COLS) focoCor -= CC_COR_COLS;
    else if (k == SDLK_DOWN && focoCor + CC_COR_COLS < n) focoCor += CC_COR_COLS;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) corAplicar(focoCor);
    return;
  }
  if (trocando) { trocadorEvento(k); return; }
  if (editando) {
    int it[64], m = ofertas(it);
    if (ehVoltar(k)) { editando = 0; return; }
    if (k == SDLK_UP && focoEd > 0) focoEd--;
    else if (k == SDLK_DOWN && focoEd + 1 < m) focoEd++;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) alternarOferta(focoEd);
    return;
  }
  {
    int n = botoes(NULL, NULL) + 1;   // + "Editar atalhos"
    if (foco >= n) foco = n - 1;
    if (ehVoltar(k)) { central_fechar(); return; }
    if (focoInfo) {   // o perfil da linha de cima: OK abre o seletor, BAIXO volta aos botoes
      if (!podeTrocar() || k == SDLK_DOWN) focoInfo = 0;
      else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) abrirTrocador();
      return;
    }
    if (k == SDLK_LEFT && foco % CC_COLS > 0) foco--;
    else if (k == SDLK_RIGHT && foco % CC_COLS < CC_COLS - 1 && foco + 1 < n) foco++;
    else if (k == SDLK_UP && foco >= CC_COLS) foco -= CC_COLS;
    else if (k == SDLK_UP && podeTrocar()) focoInfo = 1;
    else if (k == SDLK_DOWN && foco + CC_COLS < n) foco += CC_COLS;
    else if (k == SDLK_DOWN && foco / CC_COLS < (n - 1) / CC_COLS) foco = n - 1;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) ativarBotao(foco);
  }
}

// Ponteiro (Magic Remote, toque): mesmo foco e mesmo OK das setas.
static void ptFoco(int i, int ed) {
  if (ed == 3) { focoInfo = 1; return; }
  if (ed == 4) { focoPerf = i; return; }
  if (ed == 5) { pinTecla = i; return; }
  focoInfo = 0;
  if (ed == 2) focoCor = i; else if (ed) focoEd = i; else foco = i;
}
static void ptOk(int i, int ed) {
  ptFoco(i, ed);
  if (ed == 3) { abrirTrocador(); return; }
  if (ed == 4) { if (pinSlot < 0) escolherPerfil(i); return; }
  if (ed == 5) { if (!pinVerif) pinDigito(i == 9 ? 0 : i + 1); return; }
  if (ed == 2) corAplicar(i);
  else if (ed) alternarOferta(i);
  else ativarBotao(i);
}
static void ptFora(int a, int b) { (void)a; (void)b; central_fechar(); }

static void corpo(GfxRect r, float a, void *u);
static float altura(int comFaixa);

// A CENTRAL NAO TEM SUPERFICIE PROPRIA: ela e a ilha do relogio esticada
// (ilha_corpo) ou, com o player aberto, a ilha do player (plrilha_pedir), que
// ja cresce da pilula da hora. Pedido por quadro; fechada, a ilha recolhe
// sozinha chamando o corpo com o alfa caindo. Mudou a altura (botao posto ou
// tirado, edicao): a ilha anda ate a nova na mola dela.
void central_atualizar(float dt, Uint32 agora) {
  int reduz = ajustes_animacoes_reduzidas();
  (void)agora;
  if (!aberta) return;
  carregar();
  // O player abriu por baixo (ou a lista de perfis encolheu): a troca some.
  if ((trocando || focoInfo) && !podeTrocar()) { trocando = 0; focoInfo = 0; pinLimpar(); }
  if (!trocando && !editando && !escolhendoCor) {
    topoBotAjusta();
    rolaBot = anim_mola2_reduzida(&rolaBotVel, rolaBot, (float)topoBot, dt, NV_MOLA2_SCROLL, reduz);
  }
  if (trocando) {
    int n = perfis_n();
    if (focoPerf >= n) focoPerf = n - 1;
    rolaPerf = anim_mola2_reduzida(&rolaVel, rolaPerf, perfAlvoRola(), dt, NV_MOLA2_SCROLL, reduz);
    placaPerf = reduz ? (float)focoPerf : anim_mola(placaPerf, (float)focoPerf, dt, 26.0f);
    if (pinTremor > 0.0f) { pinTremor -= dt; if (pinTremor < 0.0f) pinTremor = 0.0f; }
    if (pinSlot >= 0 && pinVerif) {
      int r = atomic_load(&pinResultado);
      if (r) {
        const ContaPerfil *p = perfis_item(pinSlot);
        atomic_store(&pinResultado, 0);
        pinVerif = 0;
        memset(pinBuf, 0, sizeof pinBuf);
        if (r == 1 && p) {
          pedidoPerfil = p->indice;
          printf("[central] troca rapida para o perfil %d (PIN aceito)\n", p->indice);
          fflush(stdout);
          central_fechar();
          return;
        }
        if (r == -1) { pinErrado = 1; pinTremor = reduz ? 0.0f : 0.40f; }
        else pinRede = 1;
      }
    }
  }
  if (!player_aberto()) ilha_corpo(CC_W, altura(1), corpo, NULL);
}
// No player o pedido vai logo antes de plrilha_desenhar: o ultimo pedido com
// corpo vence, e o player faz os dele no proprio desenho. O cabecalho e o da
// plrilha ("Central de controle" e a hora); o corpo comeca abaixo dele.
void central_desenhar(Uint32 agora) {
  (void)agora;
  if (aberta && player_aberto()) {
    PlrIlhaPedido p;
    memset(&p, 0, sizeof p);
    p.texto = i18n("Central de controle");
    p.w = CC_W;
    p.h = altura(0);
    p.corpo = corpo;
    p.u = (void *)1;
    p.aberta = 1;
    p.modal = 1;
    p.ancoraTopo = 1;
    plrilha_pedir(&p);
  }
}

// --- A TECLA CH+ ---------------------------------------------------------------
int central_tecla_ocupada(void) { return chs_ocupado(&chs); }

int central_tecla(const SDL_Event *e, int pode) {
  Uint32 t;
  int r = CHS_NADA;
  if (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) return 0;
  if (!chs_ocupado(&chs) && (!pode || e->type == SDL_KEYUP)) return 0;
  t = e->key.timestamp ? e->key.timestamp : SDL_GetTicks();
  if (e->type == SDL_KEYDOWN) {
    if (!chs_ocupado(&chs)) chDown = *e;
    r = chs_desce(&chs, t);
  } else {
    // Uma linha so por sessao: quanto o CH+ deste controle leva entre descer e
    // subir. E o que diz, num log de TV, se o KEYUP vale como "soltou".
    if (!chLogado) {
      chLogado = 1;
      printf("[central] CH+ subiu %u ms depois de descer\n", (unsigned)(t - chs.tUlt));
      fflush(stdout);
    }
    chs_sobe(&chs, t);
  }
  if (r == CHS_LONGO) central_abrir();
  return 1;
}

void central_tecla_quadro(Uint32 agora, void (*entregar)(SDL_Event *e)) {
  int r;
  if (!chs_ocupado(&chs)) return;
  r = chs_quadro(&chs, agora);
  if (r == CHS_LONGO) central_abrir();
  else if (r == CHS_CURTO && entregar) {
    SDL_Event d = chDown, u = chDown;
    u.type = SDL_KEYUP;
    u.key.state = SDL_RELEASED;
    entregar(&d);
    entregar(&u);
  }
  // SEGURANDO: a barra na ilha, como o "Segure para opcoes" do OK (detail.c).
  // Os primeiros 150 ms nao contam: e um toque. Aos 600 ms a pilula ja e a
  // central (a atividade para de ser renovada e sai sozinha).
  if (!aberta && chs.estado == CHS_APERTADO && !chs.solto && agora - chs.t0 >= 150u) {
    float p = (float)(agora - chs.t0 - 150u) / (float)(CHS_SEGURAR_MS - 150u);
    ilha_atividade(i18n("Segure para abrir a central"), p > 1.0f ? 1.0f : p);
  }
}

// --- DESENHO (dentro da ilha) ----------------------------------------------------
#ifdef CENTRAL_TESTE
static char testeTit[96], testeMeta[96];
#endif

// O VIDEO QUE ESTA TOCANDO: so o que o pipeline disse (video.h). 0 = nada.
static int tocando(char *tit, size_t nt, char *meta, size_t nm) {
  const CatItem *c;
  const char *hdr, *ep;
  size_t u = 0;
  tit[0] = meta[0] = 0;
#ifdef CENTRAL_TESTE
  if (testeTit[0]) { snprintf(tit, nt, "%s", testeTit); snprintf(meta, nm, "%s", testeMeta); return 1; }
#endif
  if (previa || !player_aberto() || !player_com_video()) return 0;
  c = cat_item(player_indice());
  if (c && c->titulo[0]) snprintf(tit, nt, "%s", c->titulo);
  ep = player_linha_episodio();
  if (ep && ep[0]) u += (size_t)snprintf(meta + u, nm - u, "%s", ep);
  if (video_altura() > 0 && u < nm)
    u += (size_t)snprintf(meta + u, nm - u, "%s%dp", u ? " · " : "", video_altura());
  hdr = video_hdr();
  if (hdr && hdr[0] && strcmp(hdr, "none") && strcmp(hdr, "SDR") && u < nm)
    u += (size_t)snprintf(meta + u, nm - u, "%s%s", u ? " · " : "",
                          !strcmp(hdr, "DolbyVision") ? "Dolby Vision" : hdr);
  if (video_tem_atmos() && u < nm)
    u += (size_t)snprintf(meta + u, nm - u, "%sAtmos", u ? " · " : "");
  // DTS tocando direto ou convertido (PR #259): a faixa diz, o estado nao muda o nome.
  if (video_dts_estado() != VIDEO_DTS_NENHUM && u < nm) {
    int d = video_faixa_dts(video_audio(video_audio_atual()));
    u += (size_t)snprintf(meta + u, nm - u, "%s%s", u ? " · " : "",
                          d == 3 ? "DTS:X" : d == 2 ? "DTS-HD" : "DTS");
  }
  // "42,8 Mbps agora" (mockup, tela 1): o numero das estatisticas ao vivo,
  // com o "≈" quando e estimado.
  { int est, k = estatvivo_kbps(&est);
    if (k >= 0 && u < nm) {
      char m[24], f[48];
      ev_fmt_mbps(m, sizeof m, k, idioma_ponto_decimal(ajustes_idioma()) ? '.' : ',');
      snprintf(f, sizeof f, i18n("%s Mbps agora"), m);
      u += (size_t)snprintf(meta + u, nm - u, "%s%s%s", u ? " · " : "", est ? "≈ " : "", f);
    } }
  return tit[0] || meta[0];
}

static int linhasBotoes(void) { return (botoes(NULL, NULL) + 1 + CC_COLS - 1) / CC_COLS; }
static int linhasBotoesVis(void) { int n = linhasBotoes(); return n < CC_BOT_VIS ? n : CC_BOT_VIS; }
// Quem manda na rolagem e o foco: anda o minimo para ele ficar a vista.
static void topoBotAjusta(void) {
  int n = linhasBotoes(), f = foco / CC_COLS, maxT = n > CC_BOT_VIS ? n - CC_BOT_VIS : 0;
  if (focoInfo) f = topoBot;   // o foco subiu para a linha de cima: a grade nao se mexe
  if (f < topoBot) topoBot = f;
  if (f > topoBot + CC_BOT_VIS - 1) topoBot = f - CC_BOT_VIS + 1;
  if (topoBot > maxT) topoBot = maxT;
  if (topoBot < 0) topoBot = 0;
}
static int linhasEdicao(void) {
  int it[64], m = ofertas(it);
  return m < CC_LINHAS_MAX ? m : CC_LINHAS_MAX;
}

// ALTURA PELO CONTEUDO. `comFaixa` = a faixa de cima e da ilha (hora a
// direita); no player o cabecalho e da plrilha e a linha de informacao entra
// no corpo.
static float altura(int comFaixa) {
  char t[96], m[96];
  float h = comFaixa ? CC_INFO_H : CC_INFO_H - 12.0f;
  if (escolhendoCor) {
    h += 6.0f + 2.0f * CC_COR_D + CC_COR_LIN_GAP + 14.0f + CC_COR_NOME_H + 14.0f + CC_DICAS_H;
  } else if (trocando) {
    h += 6.0f + (pinSlot >= 0 ? CC_PIN_H : CC_PERF_CEL_H) + 14.0f + CC_DICAS_H;
  } else if (editando) {
    int n = linhasEdicao();
    h += 6.0f + (float)n * (CC_LINHA_H + CC_LINHA_GAP) - CC_LINHA_GAP + 14.0f + CC_DICAS_H;
  } else {
    int n = linhasBotoesVis();
    h += 6.0f;
    if (tocando(t, sizeof t, m, sizeof m)) h += CC_TOC_H + 10.0f;
    h += (float)n * (CC_BOTAO_H + CC_GAP) - CC_GAP;
  }
  h += CC_PAD;
  return h > CC_TETO ? CC_TETO : h;
}

static void superficie(GfxRect r, float raioPx, float f, float a) {
  if (ajustes_vidro()) gfx_cor(r, raioPx / r.h, 1, 1, 1, (0.07f + 0.10f * f) * a);
  else gfx_cor(r, raioPx / r.h, 0.135f + 0.06f * f, 0.14f + 0.06f * f, 0.163f + 0.06f * f, a);
}

// Um ponto "·" fraco entre os itens da linha de informacao.
static float sep(float x, float yc, float a) {
  gfx_cor((GfxRect){ x + 9.0f, yc - 2.0f, 4.0f, 4.0f }, 0.5f, 1, 1, 1, 0.28f * a);
  return 22.0f;
}

// A LINHA DE INFORMACAO, uma so: perfil (a cor do avatar e o nome), a rede (o
// icone diz; sem internet o icone cortado e a palavra) e a versao. A hora fica
// na ponta direita, que e a da ilha.
static void linhaInfo(float x, float yc, float wMax, float a) {
  const ContaPerfil *p = perfis_item_ativo();
  int off = rede_saude_offline();
  float x0 = x;
  if (p && p->nome[0]) {
    // O PERFIL: avatar pequeno (o mesmo desenho do menu), o nome e, quando a
    // troca existe, um chevron. E um botao: o foco e a placa clara com o anel
    // dos outros botoes.
    TxtLinha n = txt_linha_corta(TXT_ILHA_SUB, p->nome, 243, 242, 239, 255, wMax * 0.45f);
    int pode = podeTrocar();
    float wChip = CC_AV_INFO + 8.0f + (float)n.w + (pode ? 8.0f + 14.0f : 0.0f);
    GfxRect chip = { x - 8.0f, yc - 18.0f, wChip + 16.0f, 36.0f };
    if (pode) {
      if (focoInfo) {
        superficie(chip, chip.h * 0.5f, 1.0f, a);
        gfx_anel(chip, 0.5f, 2.5f, 1, 1, 1, 0.95f * a);   // por dentro: a faixa de cima nao tem folga
      }
      if (aberta && a > 0.5f && ponteiro_ativo()) ponteiro_alvo(chip.x, chip.y, chip.w, chip.h, ptFoco, ptOk, 0, 3);
    }
    avatarperfil_desenhar((GfxRect){ x, yc - CC_AV_INFO * 0.5f, CC_AV_INFO, CC_AV_INFO }, p, a);
    x += CC_AV_INFO + 8.0f;
    txt_desenhar_alpha(n, x, yc - (float)n.h * 0.5f, (focoInfo ? 1.0f : 0.9f) * a);
    x += (float)n.w;
    if (pode) {
      gfx_icone((GfxRect){ x + 8.0f, yc - 7.0f, 14.0f, 14.0f }, "aj_chevrons-down",
                0.953f, 0.949f, 0.937f, (focoInfo ? 0.95f : 0.5f) * a);
      x += 8.0f + 14.0f + 6.0f;
    }
    x += sep(x, yc, a);
  }
  gfx_icone((GfxRect){ x, yc - 9.0f, 18.0f, 18.0f }, off ? "aj_wifi-off" : "aj_wifi",
            off ? 0.94f : 0.953f, off ? 0.72f : 0.949f, off ? 0.42f : 0.937f, (off ? 1.0f : 0.7f) * a);
  x += 18.0f;
  if (off) {
    TxtLinha o = txt_linha(TXT_ILHA_HORA, "Sem internet", 240, 184, 107, 255);
    txt_desenhar_alpha(o, x + 6.0f, yc - (float)o.h * 0.5f, a);
    x += 6.0f + (float)o.w;
  }
  x += sep(x, yc, a);
  if (x - x0 < wMax - 40.0f) {
    TxtLinha v = txt_linha_corta(TXT_ILHA_HORA, NV_VERSAO, 243, 242, 239, 255, wMax - (x - x0));
    txt_desenhar_alpha(v, x, yc - (float)v.h * 0.5f, 0.45f * a);
  }
}

static void cartaoTocando(GfxRect r, const char *tit, const char *meta, float a) {
  float tx = r.x + 14.0f + 32.0f + 12.0f, tw = r.x + r.w - 14.0f - tx;
  superficie(r, 16.0f, 0.0f, a);
  gfx_icone((GfxRect){ r.x + 14.0f, r.y + (r.h - 32.0f) * 0.5f, 32.0f, 32.0f }, "aj_circle-play",
            0.953f, 0.949f, 0.937f, 0.85f * a);
  { TxtLinha t = txt_linha_corta(TXT_ILHA_ITEM, tit[0] ? tit : "Tocando agora", 243, 242, 239, 255, tw);
    TxtLinha m = txt_linha_corta(TXT_ILHA_HORA, meta, 243, 242, 239, 255, tw);
    float hy = r.y + (r.h - (float)t.h - (meta[0] ? 2.0f + (float)m.h : 0.0f)) * 0.5f;
    txt_desenhar_alpha(t, tx, hy, a);
    if (meta[0]) txt_desenhar_alpha(m, tx, hy + (float)t.h + 2.0f, 0.55f * a); }
}

// UM BOTAO: icone em cima, nome curto e, quando nao e interruptor, o valor.
// Interruptor ligado = botao cheio no acento; foco = superficie clara e anel.
static void botao(GfxRect r, const char *icone, const char *rot, const char *val, int ligado, int f, float a) {
  float ar, ag, ab, w = r.w - 24.0f, y;   // 14 a esquerda, 10 a direita
  int aceso = ligado == 1;
  int tinta = aceso ? ajustes_tinta_foco() : 243;
  float ti = (float)tinta / 255.0f;
  TxtLinha l, v;
  ajustes_acento(&ar, &ag, &ab);
  if (aceso) gfx_cor(r, CC_BOTAO_R / r.h, ar, ag, ab, a);
  else superficie(r, CC_BOTAO_R, f ? 1.0f : 0.0f, a);
  if (f) gfx_anel_fora(r, CC_BOTAO_R / r.h, 3.0f, 2.5f, 1, 1, 1, 0.95f * a);
  gfx_icone((GfxRect){ r.x + 14.0f, r.y + 14.0f, CC_ICONE, CC_ICONE }, icone, ti, ti, ti, (aceso ? 1.0f : 0.88f) * a);
  // Nome em 16/600; se nao couber (alemao, russo), um degrau menor; sem
  // valor embaixo (interruptor) e com mais de uma palavra, duas linhas em
  // 16/600 ("Choisir la / source"); o corte e o ultimo recurso.
  l = txt_linha(TXT_G16B, rot, tinta, tinta, tinta, 255);
  if ((float)l.w > w) {
    TxtLinha m = txt_linha(TXT_ILHA_HORA, rot, tinta, tinta, tinta, 255);
    if ((float)m.w <= w) l = m;
    else if (ligado >= 0 && strchr(i18n(rot), ' ')) {
      float g = gfx_opacidade_grupo, h;
      gfx_opacidade_grupo = 0.0f;
      h = txt_bloco_corta(TXT_G16B, rot, tinta, tinta, tinta, 0, 0, w, 19.0f, 1.0f, 2);
      gfx_opacidade_grupo = g;
      txt_bloco_corta(TXT_G16B, rot, tinta, tinta, tinta, r.x + 14.0f, r.y + r.h - 14.0f - h, w, 19.0f, a, 2);
      return;
    } else l = txt_linha_corta(TXT_ILHA_HORA, rot, tinta, tinta, tinta, 255, w);
  }
  y = r.y + r.h - 14.0f;
  if (ligado < 0 && val && val[0]) {
    v = txt_linha_corta(TXT_ILHA_HORA, val, tinta, tinta, tinta, 255, w);
    y -= (float)v.h;
    txt_desenhar_alpha(v, r.x + 14.0f, y, 0.6f * a);
    y -= 1.0f;
  }
  txt_desenhar_alpha(l, r.x + 14.0f, y - (float)l.h, a);
}

static void desenhaBotoes(float x, float y, float w, float a) {
  int op[CENTRAL_MAX + 1], item[CENTRAL_MAX + 1], n = botoes(op, item), i;
  int lin = linhasBotoes(), vis = linhasBotoesVis();
  float bw = (w - CC_GAP * (CC_COLS - 1)) / CC_COLS, passo = CC_BOTAO_H + CC_GAP;
  float rola = lin > vis ? rolaBot : 0.0f, hv = (float)vis * passo - CC_GAP;
  if (foco > n) foco = n;
  if (lin > vis) gfx_recorte(x - 8.0f, y - 4.0f, w + 16.0f, hv + 8.0f);
  for (i = 0; i <= n; i++) {
    GfxRect r = { x + (float)(i % CC_COLS) * (bw + CC_GAP), y + ((float)(i / CC_COLS) - rola) * passo,
                  bw, CC_BOTAO_H };
    if (lin > vis && (r.y + r.h < y - 4.0f || r.y > y + hv + 4.0f)) continue;   // fora da janela
    if (i < n && op[i] == CC_OP_ESTAT) botao(r, "aj_gauge", "Estatísticas", "", -1, i == foco && !focoInfo, a);
    else if (i < n) {
      const CentralItem *c = central_catalogo(item[i]);
      int lig = ajustes_rapido_ligado(op[i]);
      if (previa && i == previaInverte && lig >= 0) lig = !lig;
      botao(r, c->icone, c->curto, ajustes_rapido_valor(op[i]), lig, i == foco && !focoInfo, a);
      if (op[i] == AJ_RAPIDO_COR) {   // a cor de agora, num ponto no canto
        float cr, cg, cb;
        ajustes_acento_marca(&cr, &cg, &cb);
        gfx_cor((GfxRect){ r.x + r.w - 14.0f - 14.0f, r.y + 16.0f, 14.0f, 14.0f }, 0.5f, cr, cg, cb, a);
      }
    } else botao(r, "aj_sliders-horizontal", "Editar", "", 0, i == foco && !focoInfo, a);
    if (aberta && a > 0.5f && ponteiro_ativo() && r.y >= y - 8.0f && r.y + r.h <= y + hv + 8.0f)
      ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, i, 0);
  }
  if (lin > vis) {
    // a barrinha da rolagem, na margem direita do painel
    float th = hv * (float)vis / (float)lin, ty = y + (hv - th) * (rola / (float)(lin - vis));
    gfx_sem_recorte();
    gfx_cor((GfxRect){ x + w + 7.0f, y, 3.0f, hv }, 0.5f, 1, 1, 1, 0.08f * a);
    gfx_cor((GfxRect){ x + w + 7.0f, ty, 3.0f, th }, 0.5f, 1, 1, 1, 0.45f * a);
  }
}

// O SELETOR: 2 fileiras de 9 discos (claros em cima, profundos embaixo, a
// ordem da regua de Ajustes). O atual leva o visto na tinta que contrasta; o
// foco, o anel dos botoes. O nome da cor em foco fica embaixo.
static void desenhaCores(float x, float y, float w, float a) {
  int n = ajustes_rapido_cor_n(), atual = ajustes_rapido_cor_atual(), i;
  float gap = (w - CC_COR_D * CC_COR_COLS) / (CC_COR_COLS - 1);
  if (focoCor >= n) focoCor = n - 1;
  for (i = 0; i < n; i++) {
    float cr, cg, cb, tinta = ajustes_rapido_cor(i, &cr, &cg, &cb);
    GfxRect r = { x + (float)(i % CC_COR_COLS) * (CC_COR_D + gap),
                  y + (float)(i / CC_COR_COLS) * (CC_COR_D + CC_COR_LIN_GAP), CC_COR_D, CC_COR_D };
    gfx_cor(r, 0.5f, cr, cg, cb, a);
    if (i == atual)
      gfx_icone((GfxRect){ r.x + 9.0f, r.y + 9.0f, 22.0f, 22.0f }, "aj_check", tinta, tinta, tinta, a);
    if (i == focoCor) gfx_anel_fora(r, 0.5f, 3.0f, 2.5f, 1, 1, 1, 0.95f * a);
    if (aberta && a > 0.5f && ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, i, 2);
  }
  { float ny = y + 2.0f * CC_COR_D + CC_COR_LIN_GAP + 14.0f;
    TxtLinha l = txt_linha_corta(TXT_ILHA_SUB, ajustes_rapido_cor_nome(focoCor), 243, 242, 239, 255, w - 90.0f);
    txt_desenhar_alpha(l, x, ny + (CC_COR_NOME_H - (float)l.h) * 0.5f, a);
    if (focoCor == atual) {
      TxtLinha m = txt_linha(TXT_ILHA_HORA, "Atual", 243, 242, 239, 255);
      txt_desenhar_alpha(m, x + (float)l.w + 12.0f, ny + (CC_COR_NOME_H - (float)m.h) * 0.5f, 0.55f * a);
    } }
}

static void desenhaEdicao(float x, float y, float w, float h, float a) {
  int it[64], m = ofertas(it), j;
  float passo = CC_LINHA_H + CC_LINHA_GAP;
  float alvo = (float)focoEd * passo - (h - CC_LINHA_H) * 0.5f;
  float maxR = (float)m * passo - CC_LINHA_GAP - h;
  if (alvo > maxR) alvo = maxR;
  if (alvo < 0) alvo = 0;
  rolaEd = alvo;
  gfx_recorte(x - 8.0f, y, w + 16.0f, h);
  for (j = 0; j < m; j++) {
    const CentralItem *c = central_catalogo(it[j]);
    int op = ajustes_rapido_op(c->chave), tem = centrallista_tem(&lista, it[j]);
    GfxRect r = { x, y + (float)j * passo - rolaEd, w, CC_LINHA_H };
    GfxRect ic = { r.x + r.w - 14.0f - 22.0f, r.y + (r.h - 22.0f) * 0.5f, 22, 22 };
    if (r.y + r.h < y || r.y > y + h) continue;
    if (j == focoEd) superficie(r, r.h * 0.5f, 1.0f, a);
    gfx_icone((GfxRect){ r.x + 14.0f, r.y + (r.h - 22.0f) * 0.5f, 22, 22 }, c->icone,
              0.953f, 0.949f, 0.937f, (j == focoEd ? 1.0f : 0.72f) * a);
    { TxtLinha t = txt_linha_corta(TXT_ILHA_SUB, ajustes_rapido_rotulo(op), 243, 242, 239, 255,
                                   r.w - 14.0f - 22.0f - 12.0f - 50.0f);
      txt_desenhar_alpha(t, r.x + 48.0f, r.y + (r.h - (float)t.h) * 0.5f, (j == focoEd ? 1.0f : 0.72f) * a); }
    if (tem) {
      float ar, ag, ab;
      ajustes_acento_marca(&ar, &ag, &ab);
      gfx_icone(ic, "aj_circle-check", ar, ag, ab, a);
    } else gfx_icone(ic, "aj_plus", 0.953f, 0.949f, 0.937f, 0.45f * a);
    if (aberta && a > 0.5f && ponteiro_ativo()) ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, j, 1);
  }
  gfx_sem_recorte();
}

// Teclas do rodape (a peca das dicas de Ajustes, com alfa para entrar junto).
static void dicas(const char *const *k, const char *const *l, int n, float x, float y, float a) {
  int i;
  for (i = 0; i < n; i++) {
    TxtLinha tk = txt_linha(TXT_AJ_KBD, k[i], 243, 242, 239, 255);
    TxtLinha tl = txt_linha(TXT_ILHA_GENERO, l[i], 243, 242, 239, 255);
    float kw = (float)tk.w + 18.0f < 34.0f ? 34.0f : (float)tk.w + 18.0f;
    superficie((GfxRect){ x, y, kw, 28 }, 14.0f, 0.3f, a);
    txt_desenhar_alpha(tk, x + (kw - (float)tk.w) * 0.5f, y + (28.0f - (float)tk.h) * 0.5f, 0.82f * a);
    txt_desenhar_alpha(tl, x + kw + 9.0f, y + (28.0f - (float)tl.h) * 0.5f, 0.5f * a);
    x += kw + 9.0f + (float)tl.w + 20.0f;
  }
}

// A FILEIRA DE PERFIS: avatares de 64 com o nome, 4 por vez; a placa de foco
// e a rolagem andam em molas (central_atualizar). Perfil com PIN leva o escudo
// no canto do avatar; o perfil de agora, o visto na cor de destaque.
static void desenhaPerfis(float x, float y, float w, float a) {
  int n = perfis_n(), i;
  float cel = CC_PERF_CEL_W, ar, ag, ab, tinta = (float)ajustes_tinta_foco() / 255.0f;
  float x0 = x + (n < CC_PERF_VIS ? (w - (float)n * cel) * 0.5f : (w - (float)CC_PERF_VIS * cel) * 0.5f);
  float rola = n < CC_PERF_VIS ? 0.0f : rolaPerf;
  GfxRect placa = { x0 + (placaPerf - rola) * cel + 4.0f, y, cel - 8.0f, CC_PERF_CEL_H };
  ajustes_acento(&ar, &ag, &ab);
  gfx_recorte(x - 8.0f, y - 6.0f, w + 16.0f, CC_PERF_CEL_H + 12.0f);
  superficie(placa, 24.0f, 1.0f, a);
  gfx_anel_fora(placa, 24.0f / placa.h, 3.0f, 2.5f, 1, 1, 1, 0.95f * a);
  for (i = 0; i < n; i++) {
    const ContaPerfil *p = perfis_item(i);
    float cx = x0 + ((float)i - rola) * cel;
    int sel = i == focoPerf, atual = p && p->indice == perfis_ativo();
    GfxRect av = { cx + (cel - CC_PERF_AV) * 0.5f, y + 12.0f, CC_PERF_AV, CC_PERF_AV };
    float ia = (sel ? 1.0f : 0.72f) * a;
    TxtLinha nome;
    if (cx + cel < x - 8.0f || cx > x + w + 8.0f || !p) continue;
    avatarperfil_desenhar(av, p, ia);
    if (atual) {
      GfxRect b = { av.x + av.w - 22.0f, av.y + av.h - 22.0f, 22.0f, 22.0f };
      gfx_cor((GfxRect){ b.x - 2.0f, b.y - 2.0f, b.w + 4.0f, b.h + 4.0f }, 0.5f, 0.135f, 0.14f, 0.163f, a);
      gfx_cor(b, 0.5f, ar, ag, ab, a);
      gfx_icone((GfxRect){ b.x + 4.0f, b.y + 4.0f, 14.0f, 14.0f }, "aj_check", tinta, tinta, tinta, a);
    }
    if (p->temPin)
      gfx_icone((GfxRect){ av.x + av.w - 12.0f, av.y - 4.0f, 18.0f, 18.0f }, "aj_shield-check",
                0.953f, 0.949f, 0.937f, 0.9f * a);
    nome = txt_linha_corta(TXT_ILHA_SUB, p->nome[0] ? p->nome : i18n("perfil"), 243, 242, 239, 255, cel - 20.0f);
    txt_desenhar_alpha(nome, cx + ((float)cel - (float)nome.w) * 0.5f, y + 12.0f + CC_PERF_AV + 12.0f, ia);
    if (aberta && a > 0.5f && ponteiro_ativo() && cx >= x - 4.0f && cx + cel <= x + w + 4.0f)
      ponteiro_alvo(cx + 4.0f, y, cel - 8.0f, CC_PERF_CEL_H, ptFoco, ptOk, i, 4);
  }
  gfx_sem_recorte();
}

// O PIN: quem esta sendo destravado (avatar e nome ficam no titulo), quatro
// pontos (nunca os digitos), e o teclado de 5x2. Errado: os pontos balancam.
static void desenhaPin(float x, float y, float w, float a) {
  const ContaPerfil *p = perfis_item(pinSlot);
  size_t z = strlen(pinBuf), i;
  float pw = (float)CC_PIN_LEN * 18.0f + (float)(CC_PIN_LEN - 1) * 16.0f, px, py, ky;
  float tw = (float)CC_PIN_COLS * CC_PIN_TECLA_W + (float)(CC_PIN_COLS - 1) * CC_PIN_GAP;
  float kx = x + (w - tw) * 0.5f;
  if (!p) return;
  avatarperfil_desenhar((GfxRect){ x, y + (CC_PIN_CAB_H - 36.0f) * 0.5f, 36.0f, 36.0f }, p, a);
  { char t[96];
    TxtLinha l;
    snprintf(t, sizeof t, i18n("PIN de %s"), p->nome[0] ? p->nome : i18n("perfil"));
    l = txt_linha_corta(TXT_ILHA_SUB, t, 243, 242, 239, 255, w - 52.0f);
    txt_desenhar_alpha(l, x + 48.0f, y + (CC_PIN_CAB_H - (float)l.h) * 0.5f, a); }
  px = x + (w - pw) * 0.5f;
  if (pinTremor > 0.0f) px += sinf(pinTremor * 50.0f) * 14.0f * (pinTremor / 0.40f);
  py = y + CC_PIN_CAB_H + 10.0f + (CC_PIN_PONTOS_H - 18.0f) * 0.5f;
  for (i = 0; i < CC_PIN_LEN; i++) {
    GfxRect d = { px + (float)i * 34.0f, py, 18.0f, 18.0f };
    if (i < z) gfx_cor(d, 0.5f, 0.953f, 0.949f, 0.937f, a);
    else gfx_anel(d, 0.5f, 2.0f, 0.953f, 0.949f, 0.937f, (pinErrado ? 0.9f : 0.45f) * a);
  }
  if (pinErrado || pinRede) {
    TxtLinha l = txt_linha_corta(TXT_ILHA_HORA, pinRede ? i18n("Sem conexão. Tente novamente.") : i18n("PIN incorreto"),
                                 240, 184, 107, 255, w * 0.4f);
    txt_desenhar_alpha(l, x + w - (float)l.w, y + CC_PIN_CAB_H + 10.0f + (CC_PIN_PONTOS_H - (float)l.h) * 0.5f, a);
  }
  ky = y + CC_PIN_CAB_H + 10.0f + CC_PIN_PONTOS_H + 14.0f;
  for (i = 0; i < 10; i++) {
    GfxRect r = { kx + (float)(i % CC_PIN_COLS) * (CC_PIN_TECLA_W + CC_PIN_GAP),
                  ky + (float)(i / CC_PIN_COLS) * (CC_PIN_TECLA_H + CC_PIN_GAP), CC_PIN_TECLA_W, CC_PIN_TECLA_H };
    char d[2] = { (char)('0' + (i == 9 ? 0 : (int)i + 1)), 0 };
    int f = (int)i == pinTecla && !pinVerif;
    TxtLinha l = txt_linha(TXT_ILHA_ITEM, d, 243, 242, 239, 255);
    superficie(r, r.h * 0.5f, f ? 1.0f : 0.0f, (pinVerif ? 0.55f : 1.0f) * a);
    if (f) gfx_anel_fora(r, 0.5f, 3.0f, 2.5f, 1, 1, 1, 0.95f * a);
    txt_desenhar_alpha(l, r.x + (r.w - (float)l.w) * 0.5f, r.y + (r.h - (float)l.h) * 0.5f, (pinVerif ? 0.55f : 1.0f) * a);
    if (aberta && a > 0.5f && ponteiro_ativo() && !pinVerif)
      ponteiro_alvo(r.x, r.y, r.w, r.h, ptFoco, ptOk, (int)i, 5);
  }
}

// O CONTEUDO. Na ilha (u == NULL) `r` e o painel inteiro e a faixa de cima
// (CC_INFO_H) e da hora, que a ilha mantem no lugar, a direita: a linha de
// informacao (ou o titulo da edicao) fica a esquerda dela. No player (u !=
// NULL) o cabecalho e da plrilha e `r` comeca abaixo dele.
static void corpo(GfxRect r, float a, void *u) {
  float cx = r.x + CC_PAD, cw = r.w - 2.0f * CC_PAD, y, yc;
  float infoW = u ? cw : cw - 110.0f;   // a hora ocupa a ponta direita da faixa
  Uint32 agora = SDL_GetTicks();
  if (a < 0.01f) return;
  if (aberta && a > 0.5f && ponteiro_ativo()) {
    ponteiro_camada();
    ponteiro_alvo(0, 0, NV_TELA_W, NV_TELA_H, NULL, ptFora, 0, 0);
    ponteiro_alvo(r.x, r.y, r.w, r.h, NULL, NULL, 0, 0);
  }
  yc = u ? r.y + (CC_INFO_H - 12.0f) * 0.5f : r.y + CC_INFO_H * 0.5f;
  y = r.y + (u ? CC_INFO_H - 12.0f : CC_INFO_H) + 6.0f;

  if (escolhendoCor) {
    float base = r.y + r.h - CC_PAD - CC_DICAS_H;
    ajustes_ui_kicker(i18n("Cor de destaque"), cx, yc - 8.0f, a);
    desenhaCores(cx, y, cw, a);
    if (aviso[0] && (Sint32)(avisoAte - agora) > 0) {
      TxtLinha l = txt_linha_corta(TXT_ILHA_HORA, aviso, 240, 184, 107, 255, cw);
      txt_desenhar_alpha(l, cx + cw - (float)l.w, base + (CC_DICAS_H - (float)l.h) * 0.5f, a);
    }
    { static const char *const tk[] = { "OK", "Voltar" };
      static const char *const rot[] = { "Escolher", "Pronto" };
      dicas(tk, rot, 2, cx, base, a); }
    return;
  }

  if (trocando) {
    float base = r.y + r.h - CC_PAD - CC_DICAS_H;
    ajustes_ui_kicker(i18n("Trocar de perfil"), cx, yc - 8.0f, a);
    if (pinSlot >= 0) desenhaPin(cx, y, cw, a);
    else desenhaPerfis(cx, y, cw, a);
    { static const char *const tk[] = { "OK", "Voltar" };
      static const char *const rotL[] = { "Escolher", "Pronto" };
      static const char *const rotP[] = { "Escolher", "Voltar" };
      dicas(tk, pinSlot >= 0 ? rotP : rotL, 2, cx, base, a); }
    return;
  }

  if (editando) {
    char t[24];
    float kw = ajustes_ui_kicker(i18n("Editar atalhos"), cx, yc - 8.0f, a);
    float base = r.y + r.h - CC_PAD - CC_DICAS_H;
    snprintf(t, sizeof t, "%d / %d", lista.n, CENTRAL_MAX);
    { TxtLinha l = txt_linha(TXT_ILHA_HORA, t, 243, 242, 239, 255);
      txt_desenhar_alpha(l, cx + kw + 12.0f, yc - (float)l.h * 0.5f, 0.55f * a); }
    desenhaEdicao(cx, y, cw, base - 14.0f - y, a);
    if (aviso[0] && (Sint32)(avisoAte - agora) > 0) {
      TxtLinha l = txt_linha_corta(TXT_ILHA_HORA, aviso, 240, 184, 107, 255, cw);
      txt_desenhar_alpha(l, cx + cw - (float)l.w, base + (CC_DICAS_H - (float)l.h) * 0.5f, a);
    }
    { static const char *const tk[] = { "OK", "Voltar" };
      static const char *const rot[] = { "Pôr ou tirar", "Pronto" };
      dicas(tk, rot, 2, cx, base, a); }
    return;
  }

  linhaInfo(cx, yc, infoW, a);
  { char tit[96], meta[96];
    if (tocando(tit, sizeof tit, meta, sizeof meta)) {
      cartaoTocando((GfxRect){ cx, y, cw, CC_TOC_H }, tit, meta, a);
      y += CC_TOC_H + 10.0f;
    } }
  desenhaBotoes(cx, y, cw, a);
  // Falha ao gravar: some sozinha; mora na faixa de cima, no lugar da linha.
  if (!previa && aviso[0] && (Sint32)(avisoAte - agora) > 0) {
    TxtLinha l = txt_linha_corta(TXT_ILHA_HORA, aviso, 240, 184, 107, 255, cw);
    txt_desenhar_alpha(l, cx, r.y + r.h - CC_PAD * 0.5f - (float)l.h, a);
  }
}

// A PREVIA (novidades201.c): o mesmo desenho da central aberta, com os
// atalhos de fabrica, sem ler nem gravar a lista da pessoa, sem alvo de
// ponteiro e sem mexer no foco dela.
static void previaEntrar(CentralLista *salva, int *est) {
  *salva = lista;
  est[0] = foco; est[1] = aberta; est[2] = editando;
  centrallista_padrao(&lista);
  aberta = 0; editando = 0; previa = 1;
}
static void previaSair(const CentralLista *salva, const int *est) {
  lista = *salva;
  foco = est[0]; aberta = est[1]; editando = est[2];
  previa = 0; previaInverte = -1;
}
float central_previa_altura(void) {
  CentralLista s;
  int e[3];
  float h;
  previaEntrar(&s, e);
  h = altura(1);
  previaSair(&s, e);
  return h;
}
void central_previa_desenhar(float x, float y, float a, int f, int inverte) {
  CentralLista s;
  int e[3];
  previaEntrar(&s, e);
  foco = f;
  previaInverte = inverte;
  corpo((GfxRect){ x, y, CC_W, altura(1) }, a, NULL);
  previaSair(&s, e);
}

float central_rotulo_w(void) {
  return (CC_W - 2.0f * CC_PAD - CC_GAP * (CC_COLS - 1)) / CC_COLS - 24.0f;   // o `w` de botao()
}

#ifdef CENTRAL_TESTE
// A troca rapida: `sim` liga a troca mesmo sem player parado; o foco na linha
// de cima; o seletor aberto no slot `f`, e dele o PIN com `digitos` pontos.
void central_teste_trocavel(int sim) { testePodeTrocar = sim; }
void central_teste_info(int sim) { focoInfo = sim && podeTrocar(); }
void central_teste_trocar(int f, int pin, int digitos) {
  testePodeTrocar = 1;
  central_abrir();
  editando = escolhendoCor = 0;
  abrirTrocador();
  focoPerf = f; placaPerf = (float)f; rolaPerf = perfAlvoRola();
  if (pin >= 0) {
    pinSlot = pin; pinTecla = 4;
    memset(pinBuf, 0, sizeof pinBuf);
    while (digitos-- > 0 && strlen(pinBuf) < CC_PIN_LEN) pinBuf[strlen(pinBuf)] = '7';
  }
}
void central_teste_tocando(const char *tit, const char *meta) {
  snprintf(testeTit, sizeof testeTit, "%s", tit ? tit : "");
  snprintf(testeMeta, sizeof testeMeta, "%s", meta ? meta : "");
}
void central_teste_estatisticas(int sim) { testeEstat = sim; }
void central_teste_lista(const char *texto) { carregar(); centrallista_ler(&lista, texto); }
void central_teste_foco(int f, int editar) {
  central_abrir();
  editando = escolhendoCor = 0;
  trocando = 0; focoInfo = 0; pinSlot = -1;
  foco = f < 0 ? botoes(NULL, NULL) : f;
  topoBot = 0; topoBotAjusta(); rolaBot = (float)topoBot; rolaBotVel = 0.0f;
  if (editar) ativarBotao(botoes(NULL, NULL));
}
// Captura do seletor de cores, com o foco no disco `foco`.
void central_teste_cor(int f) { central_abrir(); editando = 0; escolhendoCor = 1; focoCor = f; }
#endif
