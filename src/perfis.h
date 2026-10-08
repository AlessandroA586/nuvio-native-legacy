// Perfis da conta.
//
// Tudo que o sync pede leva `p_profile_id`. Sem escolher um perfil o app
// sincronizaria o perfil 1 sempre — e numa conta de familia isso significa
// mostrar a lista de outra pessoa e, pior, ESCREVER o progresso dela. Por isso
// os perfis vem antes de qualquer outra superficie.
//
// O DONO DA CONTA nao e necessariamente quem logou: `get_sync_owner` devolve o
// id de quem realmente possui os dados (conta compartilhada). MEDIDO: a RPC
// existe e responde uma string JSON crua com o uuid. A leitura da tabela de
// addons filtra por ESSE id, nao pelo `sub` do token.
#ifndef NV_PERFIS_H
#define NV_PERFIS_H

#define CONTA_PERFIL_MAX 8

typedef struct {
  int  indice;          // profile_index (1..n) — e o que vai em p_profile_id
  char nome[64];
  char corHex[10];      // avatar_color_hex, "#1E88E5"
  // MEDIDO nesta conta: `avatar_url` vem NULO e o `avatar_id` ("avatar_lalo")
  // so vira imagem pela tabela `avatars`, que NAO existe neste servidor
  // (PGRST205). Ou seja: quando nao ha url, nao ha foto para buscar — o
  // circulo com a inicial e a representacao, nao um remendo.
  char avatarUrl[300];
  // profile_background_url: a arte de FUNDO deste perfil, do mesmo
  // `mapProfileRow` (secao 1.5 do PLANO-CONTA-SYNC). E o que da a cada perfil
  // uma tela propria em vez de uma grade de circulos coloridos. Como o avatar,
  // e uma URL baixada pelo tex_cache — nenhuma arte de pessoa entra no pacote,
  // que e o que a checagem de tools/arm.sh existe para impedir.
  char fundoUrl[300];
  int  primario;        // is_primary
  int  temPin;          // veio de sync_pull_profile_locks
  // uses_primary_plugins: este perfil LE os addons do perfil 1 em vez de ter
  // os seus. Ver perfis_ativo_addons().
  int  usaAddonsDoPrimario;
} ContaPerfil;

// Busca os perfis e o dono. BLOQUEIA — chamar do fio de sync.
// Devolve quantos achou. Zero NAO e erro: conta nova pode nao ter perfil
// nenhum criado, e nesse caso o app opera com o perfil 1 implicito.
int perfis_puxar(void);

int           perfis_n(void);
const ContaPerfil *perfis_item(int i);
// O perfil em vigor, ou NULL quando a lista ainda nao chegou.
const ContaPerfil *perfis_item_ativo(void);
const char   *perfis_dono(void);        // uuid de get_sync_owner; "" se nao veio

// ContaPerfil ativo. Persistido em disco: reescolher a cada arranque seria uma
// pergunta que o app ja sabe responder.
int  perfis_ativo(void);                // profile_index; 1 quando nada escolhido
// O perfil de onde SAEM OS ADDONS: igual a perfis_ativo(), exceto quando o
// perfil herda os do primario (uses_primary_plugins), e ai e 1.
int  perfis_ativo_addons(void);
void perfis_definir_ativo(int indice);
// Le do disco o perfil ativo E A LISTA DE PERFIS DA ULTIMA SESSAO. Chamar uma
// vez no arranque, antes de app_iniciar.
//
// A LISTA TAMBEM E CACHEADA, e nao so o indice ativo. Sem ela a tela de escolha
// nao tem o que desenhar antes de o sync responder: so poderia aparecer
// segundos depois, POR CIMA de uma home ja visivel — que e como ela se
// comportava. Com o cache ela abre no PRIMEIRO quadro, com os nomes e as cores
// certos, e o sync so confirma. O arquivo e do usuario, mora na pasta de dados
// e some em perfis_esquecer() junto com o resto.
void perfis_carregar_ativo(void);

// 1 quando a tela de escolha tem uma pergunta DE VERDADE a fazer nesta sessao.
//
// A regra, e o porque de cada linha:
//   - nenhum perfil      -> 0. Conta nova opera com o perfil 1 implicito;
//                           perguntar seria uma tela sem resposta possivel.
//                           Vale tambem para quem NAO tem conta: sem sessao
//                           nao ha lista, entao nada aparece.
//   - um perfil, sem PIN -> 0. Uma pergunta com uma resposta so nao e pergunta,
//                           e numa TV ela custa um clique a cada abertura.
//   - um perfil COM PIN  -> 1. A trava existe justamente para o aparelho nao
//                           abrir o perfil sozinho; pular aqui seria desliga-la.
//   - dois ou mais       -> 1, UMA VEZ POR SESSAO. Numa TV de sala quem liga o
//                           aparelho hoje nao e necessariamente quem o desligou
//                           ontem, e herdar a escolha de ontem em silencio e
//                           exatamente o que faz o app gravar o progresso no
//                           perfil errado.
//
// "Uma vez por sessao" e o que separa isto de um obstaculo: depois de escolher
// — ou de dispensar com o Voltar — ela devolve 0 pelo resto da execucao, entao
// voltar para a home nunca reabre a pergunta.
int  perfis_precisa_escolher(void);

// A MESMA condicao, SEM a bandeira de sessao: 1 quando a lista de agora nao
// oferece escolha nenhuma (zero perfis, ou um so e destravado).
//
// Existe porque a tela tambem e aberta DE PROPOSITO pelo "trocar de perfil" do
// menu. Ali a bandeira de sessao ja esta ligada, entao perguntar
// perfis_precisa_escolher() faria a tela se dispensar sozinha no quadro
// seguinte. Duas funcoes e nao duas copias da regra: esta e a regra, e
// perfis_precisa_escolher() e ela mais a bandeira.
int  perfis_sem_escolha(void);

// 1 quando o Voltar pode dispensar a tela sem escolha explicita: ha um perfil
// gravado da sessao anterior E ele nao esta atras de um PIN. Sem esta segunda
// condicao o Voltar contornaria a trava.
int  perfis_pode_dispensar(void);

// Confirma o perfil que ja estava gravado, sem reescrever o arquivo. E o que o
// Voltar faz: encerra a pergunta desta sessao mantendo a resposta de ontem.
void perfis_manter_ativo(void);

// Qual SLOT (0..n-1 da lista, nao o profile_index) a tela comeca focando: o do
// perfil ativo, ou 0 quando ele nao esta mais na lista. Abrir a tela com o
// cursor no primeiro perfil sugeriria que a escolha se perdeu.
int  perfis_indice_sugerido(void);

// O que a tela deve fazer ao apertar OK sobre o slot `i`. Existe como funcao
// separada — em vez de um `if (p->temPin)` dentro da tela — porque e a regra
// que um teste sem SDL consegue provar.
typedef enum {
  PERFIL_ACAO_NADA = 0,   // slot que nao existe
  PERFIL_ACAO_ENTRAR,     // pode entrar direto
  PERFIL_ACAO_PIN         // travado: pedir o PIN antes
} PerfilAcao;
PerfilAcao perfis_acao(int i);

// Valida o PIN de um perfil travado. BLOQUEIA.
//
// TRES respostas, e a terceira e o motivo de isto nao ser um booleano:
//    1  o servidor aceitou
//    0  o servidor recusou — o PIN esta errado
//   -1  nao deu para perguntar (sem rede, ou o servidor respondeu erro)
//
// Juntar -1 com 0 faz a TV sem rede dizer "PIN incorreto" a quem digitou o PIN
// certo, e manda essa pessoa tentar de novo para sempre.
int  perfis_verificar_pin(int indice, const char *pin);

// ENTRAR AUTOMATICAMENTE COM UM PERFIL (4a funcao da tela de escolha: nao
// aparecer). Um unico perfil por TV pode ter a marca: ligar em outro desliga a
// anterior, porque e UM indice, nao uma bandeira por perfil. Fica nesta TV
// (perfil-auto.txt): a conta nao tem coluna para isso (sync_pull_profiles
// devolve nome, avatar, cor, is_primary, uses_primary_addons e a trava, e so).
//
// 0 = nenhum. Nunca devolve um perfil que saiu da conta nem um com PIN: a
// trava existe para o aparelho nao abrir o perfil sozinho.
int  perfis_auto(void);
// 1 quando o perfil `indice` PODE receber a marca: esta na lista e nao tem PIN.
int  perfis_auto_pode(int indice);
// Liga (indice > 0) ou desliga (0) a marca. Recusa PIN e perfil desconhecido
// (devolve 0 e nada muda). Ligar num perfil tira a marca do anterior.
int  perfis_auto_definir(int indice);
// O perfil em que o arranque deve entrar sem perguntar, ou 0 quando a pergunta
// vale (nenhuma marca, perfil travado/ausente, ou a pessoa ja escolheu).
int  perfis_auto_candidato(void);

// Esquece os perfis, o dono e a escolha gravada. Chamado ao SAIR: manter a
// escolha faria a conta seguinte comecar sincronizando o `p_profile_id` da
// conta anterior — ou seja, ESCREVENDO progresso no perfil errado.
void perfis_esquecer(void);

#endif
