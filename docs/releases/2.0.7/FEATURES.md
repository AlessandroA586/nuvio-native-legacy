# Nuvio Legacy 2.0.7

Everything merged into `integracao/2.0.5` after the `v2.0.6` tag (base `ae9c3c138`), plus the owner requests
that landed in the same window. Last tracking sync: 2026-10-17 08:16, HEAD `ab0e9a597`. Community server:
**https://discord.gg/9NWr6SHyzJ**

Release engineering is already merged on `agente/205-release207` (`b72189c5d`): version bumped to 2.0.7 in
`deploy/app/appinfo.json` and `tools/tizen-config.xml`, the what's-new card (`src/novidades_cartao.c`) points
at 2.0.7 and `docs/releases/2.0.7/NOTAS.md` holds the GitHub release notes. Before tagging, `repo.json` still
needs the built .ipk (url, sha256, size) and the frost performance line below has to be filled in.

> O cartao dentro do app (`src/novidades/207.inc`) tem uma lista menor, escrita em conjunto com o dono;
> este arquivo e a fonte completa para as release notes do GitHub.

---

## PT-BR

### Destaques

- **Deep links** (#539): `nuvio-legacy://`, `stremio://` e links do IMDb abrem o app direto no título,
  na busca ou já tocando, com o link mais recente guardado enquanto a descoberta roda. Todas as plataformas.
  Refs #539, `8f38bb943`, `a3eb1e19a`, `7cf145dca`, `f44e64863`, `a4f220cd7`.
- **Cache do P2P no pendrive, em todas as plataformas** (pedidos do dono, #486 #505): o cache USB passa a
  aparecer e funcionar no Android e no Samsung .tpk — antes só no webOS — com o tamanho e o motivo de cada
  recusa explicados em português. Android, Samsung .tpk. Refs pedidos do dono, `31a62b273`, `51daddd84`,
  `b090905dc`, `a633888a3`, `ebb5eb8fe`, `9b4f37efb`.
- **Player externo no Android TV** (#501): escolha qual app de vídeo instalado abre a fonte em
  *Ajustes › Player* (Interno, Externo ou Perguntar sempre) — e uma escolha separada para anime. Legenda e
  proporção sobrevivem. Android. Refs #501, `f954a272c`, `276207fac`, `059f4d4f3`.
- **Modo TV e Celular automático no APK Android** (PR #431): o app escolhe a interface pelo tamanho da tela e
  pelo controle, sem alternar nada na mão. Android. Refs PR #431, `c305e6f7e`, `8979ef16c`, `875d7f18f`.
- **Legenda com contorno nítido** (pedidos do dono): o contorno da legenda passa a ser um anel opaco de
  8 direções, sem o "sombra" translúcida, e a opacidade escolhida vale uma vez só em vez de se acumular nas
  quinas. LG webOS, Samsung .tpk. Refs `6a76a46aa`, `d0f6f5986`, `f29e252c2`.
- **Simkl com anime** (pedidos do dono): o terceiro tipo do Simkl (anime) entra nas listas de séries, no
  Continuar assistindo e no Planejo assistir, sem itens repetidos. Todas as plataformas. Refs `f5dab2f8a`,
  `d6244a1f1`.
- **Especiais (temporada 0) aparecem e tocam** (#556, PR #569): animes com temporada especial ou fora de
  ordem, que o Trakt reconhece, deixam de ser ignorados; a numeração do provedor continua valendo na fonte, na
  retomada, no próximo episódio e no Trakt/Simkl/Plex, e a temporada fake que o add-on cria ("The Blacklist:
  Season 20+") não aparece mais. Todas as plataformas. Refs #556, PR #569, `8dff0e2a9`, `4a443422b`,
  `94d16f7de`, `288e531ee`, `054ccef24`.
- **Vidro fosco mais rápido com o mesmo visual** (pedido do dono): o desfoque do vidro passou a trabalhar
  só nos retângulos que os paineis leem e no máximo em 960x540, com 3 passos em vez de 5 e sem recaptura
  atrasada depois de um corte de cena. No Home da C9 a 1080p o desfoque caiu de 498.960 para 24.289 pixels
  por quadro e as 12 imagens de referência não passam de 4 de diferença. Todas as plataformas.
  Refs pedido do dono, `4c82273d7`, `93b3d69ec`, `3dc70083c`, `013e1341f`, `fe6a04804`, `4dba47b60`.
- **Fileira de amigos polida, sem "Ver todos"/"+ Adicionar"** (pedido do dono): todos os 6 estilos da fileira
  ganham o foco padrão da Home (sem o contorno vermelho), cartaz em paisagem com o texto no lugar certo,
  avatar com a inicial quando não tem foto, cartaz de reserva quando não tem arte e os dois botões da fileira
  saem. Todas as plataformas. Refs pedido do dono, `8806e6aff`, `d95bd32e1`, `e27514693`, `f21d993b2`.
- **Marcar como visto funciona** (#548): marcar a série inteira ou um episódio continua valendo depois que
  outro processo antigo responde, e os gestos confirmados sobrevivem a reiniciar o app e a um Trakl que puxa
  estado velho. Todas as plataformas. Refs #548, `66a608ef7`, `8330673a8`, `2d04fbecc`, `b074d9fd5`,
  `91da5935e`.
- **Tela de abertura do player** (pedido do dono): a logo desliza para o centro e o botão Play vira um anel
  fino de progresso embaixo dela, sobre a arte nítida com um véu escuro subindo até a metade; "Arte" é o novo
  padrão, com "Arte borrada" e "Desligado" ainda disponíveis. **Estilo** escolhe entre **Centro (A3)** e
  **Canto (B3)**, com a logo embaixo à esquerda e o anel fino sob ela. Todas as plataformas.
  Refs pedido do dono, `718e61abb`, `ace1fd53a`, `922500ef4`, `9c3ecdf85`.
- **Fundo Cinza ou Cor no layout Dinâmico** (pedido do dono): o fundo do layout Dinâmico (o estilo
  "Apple TV") deixa de ser só arte e passa a aceitar Cinza ou Cor, escolhido na paleta de acento que a
  interface já usava, com o Imersivo mais calmo. Todas as plataformas.
  Refs pedido do dono, `6da1fc418`, `e91329dd2`, `0cf805980`, `e3fdbe717`.
- **Frost da barra lateral e das ilhas da Home** (pedido do dono): a barra lateral desfoca sobre as telas
  que carregam vidro, a pílula de acento vira vidro tingido no modo vidro, as ilhas de frost deixam de ser
  desenhadas por baixo do vidro e os modais passam a capturar só a região que o painel lê — inclusive o
  "Salvo". Todas as plataformas.
  Refs pedido do dono, `90b0f9cd7`, `61764f975`, `fb4af3ee5`, `f087f4b59`, `7bf4caac9`, `e3eb56d39`,
  `37687dce0`, `90c8d380d`, `ab0e9a597`.
- **Ficha de fontes pelo relógio**: a folha de fontes agora nasce da ilha do relógio, com a mesma mola e o
  mesmo material do resto da interface. Todas as plataformas. `fbe06f5e7`, `ae39b748a`.
- **Ver tudo em grade** (PR #568 #563): as fileiras de destaque das coleções e os resultados de busca de cada
  catálogo abrem em grade com todos os itens, sem o limite da fileira. Todas as plataformas.
  Refs PR #568 #563, `4baf783fa`, `e0ddeca5f`.
- **Catálogo fora da cota atualiza na hora** (follow-up de #545): a fileira que você acabou de adicionar
  renova sozinha, sem depender de outra ação. Todas as plataformas. `975638744`, `53b050753`, `b03502f84`.
- **Busca e episódios voltam a funcionar** (#555): séries de título exato como "Bigg Boss" não ficam mais fora
  do limite de resultados, e séries que vêm de um add-on sem episódios preenchem a lista pelo catálogo.
  Todas as plataformas. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catálogos: fileiras fantasmas e fontes que somem** (#545 #551 #552): catálogos de add-on desinstalado saem
  da Home, fileiras que o add-on não declara nesta sessão não ocupam vaga, e as fontes voltam a aparecer
  quando a sonda do add-on demora. Todas as plataformas. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`,
  `455135df2`, `cbcf467f5`, `27988615e`.

### Novidades

- **Estilo do título das fileiras**: *Ajustes › Home › Títulos das fileiras* escolhe entre
  Classico, Ilha (título numa pílula de vidro com o ícone e a contagem), Editorial (linha pequena em
  caixa alta acima do título) e Acento (ponto que vira barra na fileira em foco, com "Ver tudo" no fim).
  Todas as plataformas. Refs pedido do dono, `331746034`, `4898972d7`.
- **Cartaz grande em três tamanhos**: a fileira de cartaz com informação ganha Pequeno, Médio e Grande
  (*Ajustes › Aparência da Home › Estilo de linha*, esquerda/direita como Paisagem e 4:3) — Pequeno com
  1 linha de sinopse, Grande com 3 e a altura que sobra virando largura. O Médio continua idêntico.
  Todas as plataformas. Refs pedido do dono, `e50a38ef6`, `cbf6fb021`.
- **Vidro defumado sobre o vídeo**: no player e nos painéis sobre a imagem ao vivo o vidro deixa de
  desfocar e passa a ser tinta escura translúcida, sem captura de tela. Todas as plataformas.
  Refs pedido do dono, `6f1c61cb8`, `a691e96fb`.
- **Qualidade do vidro automática**: *Ajustes › Qualidade do vidro* (Automática, Desempenho, Alta) — no
  Desempenho o app usa só o vidro defumado, sem captura nem desfoque; na Automática ele cai sozinho para
  o defumado quando a TV trava 4 janelas seguidas de 150 quadros. Todas as plataformas.
  Refs pedido do dono, `30a932909`, `a691e96fb`.
- **Abertura do Dolby Vision no estilo novo**: a tela de Cinema do Dolby Vision passa a usar a mesma
  abertura das outras telas, com Estilo (Centro/Canto), Fundo (Desligado/Arte/Borrada), linha do add-on,
  card detalhado e animações reduzidas valendo igual. Todas as plataformas.
  Refs pedido do dono, `d89784e82`, `8f788b25b`, `b59987511`, `ef33525a8`, `4542aeffe`, `abb9e49b9`.
- **Frost no player e nos menus**: os botões, a barra de controle, os painéis de fonte/legenda/áudio, o
  painel de Estatísticas ao vivo e os menus rápido e de guia parental agora usam o mesmo vidro fosco do
  resto da interface, em vez de painel chapado. Todas as plataformas (onde o vídeo dá para ler pelo GL).
  Refs pedidos do dono, `46e0bd5bd`, `3982ef84b`.
- **Menu do topo translúcido com o frost desligado**: com o Vidro ligado e o "Vidro fosco" desligado, a
  barra do topo (e a ilha de avatar/relógio) fica transparente e tingida em vez de opaca. LG webOS.
  `b5007ae85`.
- **Home que acompanha seus ajustes sem reiniciar** (#468, pedido do dono): reordenar ou desativar fileiras
  em Ajustes e adicionar um servidor Jellyfin/add-on novo atualiza a Home na hora, e um servidor com muitas
  bibliotecas deixa de mostrar só quatro catálogos — o teto subiu de 6 para 16 visões, com boxsets e visões
  sem tipo aceitas. Todas as plataformas. Refs #468, `b0e429762`, `c47a40788`, `08c98b017`.
- **"Assistir do começo" e Recarregar no lugar certo** (#562, PR #574): o botão da barra do player passa a
  voltar para 0:00 sem perder URL, pausa nem faixas, e o "Recarregar" da fonte sai da barra e entra na folha
  de fontes, confirmado só quando você solta o botão. Todas as plataformas.
  Refs #562, PR #574, `6a6d5b71b`, `075fec9a4`, `2c7c85860`.
- **"Play from beginning" and Reload in the right place** (#562, PR #574): the player toolbar button now
  seeks back to 0:00 without losing the URL, pause state or tracks, and the source "Reload" leaves the toolbar
  and moves into the Sources sheet, committed only on key-up. All platforms.
  Refs #562, PR #574, `6a6d5b71b`, `075fec9a4`, `2c7c85860`.
- **Um botão a menos no player** (pedido do dono): o botão de voltar episódio sai da barra de ações das
  séries; o de avançar e todos os outros ficam, com a ordem de foco e o espaçamento intactos. Todas as
  plataformas. Refs pedido do dono, `4e6b592d8`, `d51d4e46c`.
- **Linha "Cache do P2P" no Android**: a configuração de cache em pendrive aparece e funciona no APK
  Android, como já acontecia no webOS. Android. `9b4f37efb`.
- **Ver tudo em grade nas coleções**: as fileiras de destaque abrem em grade de "Ver tudo" com todos
  os itens da coleção. Todas as plataformas. `4baf783fa`.
- **Episódio certo no scrobble**: o progresso enviado para o Trakt usa a identidade exata do episódio.
  Todas as plataformas. Refs PR #560, `ff1f1aa25`.
- **Identidade da série preservada** (#524, PR #561): a série continua a mesma ao ser buscada e ao tocar, em
  vez de virar outro título, e o episódio pedido é o que toca — vale também para o Continuar assistindo e os
  cartões de retomada. Todas as plataformas. Refs #524, PR #561, `cdf5bdd88`, `2537db6a0`, `86d194462`.
- **Continuar assistindo com até 40 itens** (PR #538): o limite nativo subiu de 12 para 40 e fileiras futuras
  são publicadas até o teto. Todas as plataformas. Refs PR #538, `4cbf5ad5c`, `696a3a6ee`, `cee137de7`.
- **Tamanho do buffer** (*Ajustes › Tamanho do buffer*): Automático, 64, 128 ou 256 MiB, sempre limitado pela
  memória livre. Todas as plataformas. `42b3718cf`, `62421bfdc`, `35a944d9f`.
- **Linha "Split hero"** (*Ajustes › Aparência da Home › Estilo de linha*): o título focado vira um painel
  grande com arte e sinopse. Todas as plataformas. `8655956cb`, `669fa7b5c`, `f8815746a`, `76c6e8eea`.
- **Som do trailer**: *Trailer áudio* escolhe Automático, Original ou um idioma da lista, e prefere o trailer
  dublado quando existe. Todas as plataformas. `b43655122`, `c831a592f`, `c0ea07466`, `31dcf5275`.
- **Legenda no trailer do YouTube** (webOS): o trailer embutido segue o idioma da legenda. LG webOS (wasm).
  `e23f0476d`.
- **Continuar assistindo até 40 itens**: a fileira publica todos os itens que cabem, em vez de parar em 12.
  Todas as plataformas. PR #538, `4cbf5ad5c`, `696a3a6ee`, `cee137de7`.
- **Seletor de estilo de linha animado**: abrir e fechar o seletor de linha agora tem animação suave e o toque
  errado durante o fechamento não troca mais nada. Todas as plataformas. `92656f2ae`, `84239e65a`,
  `40b7d8e15`, `86c763a2d`.
- **Relógio com Up**: das telas de navegação, Up chega no relógio e na ilha de retomada (PR #547).
  Todas as plataformas. `a7d8b1f58`, `960ec872a`.
- **Reordenar blocos de add-on no celular** e **salvar com aviso na hora**: os Ajustes do APK no modo Celular
  mostram o que só existe na TV e confirmam o salvamento. Android. `4bd7a33cc`, `acc47411f`, `29a516106`,
  `7163e053c`, `510007bdd`.
- **Elenco e metadados mais confiáveis**: retratos do add-on ficam quando o TMDB falha, nomes com acento e
  emojis não viram duplicatas, e backdrops rasos de recomendação são trocados pelo de alta qualidade.
  Todas as plataformas. PR #540 PR #546, `773c576f7`, `a0e225043`, `946ad1819`.
- **Áudio e fonte lembrados**: a faixa de áudio escolhida vale para a série inteira, inclusive depois de
  fechar o app (PR #550). Todas as plataformas. `0c2a1ba2c`.
- **Log dos amigos em inglês**: o motivo da fileira de amigos não suja mais a varredura de tradução.
  Todas as plataformas. `171e49772`.

- **Abas da Biblioteca e do catalogo com vidro de verdade** (pedido do dono): as tiras de aba
  (Salvos/Colecao/Listas, Filmes/Series, a pilula do titulo e os segmentados restantes) passam a usar o mesmo
  vidro fosco do resto do app, com a aba escolhida mantendo a cor de destaque. Todas as plataformas.
  Refs pedido do dono, `21696fd8c`, `796a3441f`, `a096e5eb8`.
- **Ajustes em 100% por padrao** (pedido do dono): a escala dos Ajustes vem de fabrica em 100% em vez de 80%;
  quem nunca escolheu um tamanho na mao passa para 100% e a escolha explicita de cada um fica como estava.
  Todas as plataformas. `3f8d304b7`, `f22761e28`.
- **Menu contextual com vidro e frost de verdade** (pedido do dono): o menu contextual grande e o pequeno
  passam a desfocar a arte que está atrás do painel, em vez de mostrar uma chapa cinza; *Ajustes › Menu do
  cartaz* escolhe entre **Completo** (oPainel enriquecido, novo padrão) e **Simples** (o comportamento antigo,
  só o menu ao lado do cartaz). Todas as plataformas. Refs pedido do dono, `a2ea074c7`, `dc040ef2d`.
- **Context menu with real glass and frost** (owner request): the big and the small context menu now blur
  the artwork behind the panel instead of showing a flat gray slab; *Settings › Poster menu* picks
  **Full** (the enriched panel, new default) or **Simple** (the old behaviour, just the menu beside the
  poster). All platforms. Refs owner request, `a2ea074c7`, `dc040ef2d`.
- **Arte em alta qualidade que sobe de tamanho** (pedidos do dono): com a qualidade Alta, pôsteres w342 e
  backdrops w780 guardados pelo catálogo passam a ser buscados no tamanho que a tela precisa, e o backdrop
  pequeno de 480px não é mais servido para cartões de até 640px. Todas as plataformas. `ff1cd6dcd`,
  `44bf4884a`.
- **Modelo de URL de pôster longo** (pedidos do dono): colar um modelo de URL longo na página do celular
  deixa de ser cortado em 1024 caracteres, e as sessões nativas acompanham o mesmo limite. Todas as
  plataformas. `0950a8eb8`, `df7c93aaf`.
- **Faixa de áudio MKV lembrada**: em MKV com faixa convertida, a escolha manual continua valendo para o
  episódio seguinte, mesmo com duas faixas idênticas. Todas as plataformas. `3486c616b`.

### Correções

- **Fundo Dinâmico sem piscar**: com o vidro fosco ligado no layout Dinâmica, o fundo atrasado é
  resolvido antes da cópia do frost — a Home para de alternar entre a arte certa e faixas coloridas no
  Android. Android. Refs pedido do dono, `5528dc0ad`, `6a64c2e13`.
- **Hero dividido sem título repetido**: no hero dividido a lista de cartazes deixa de repetir o título que
  já está em foco. Todas as plataformas. Refs pedido do dono, `4428dea18`, `f41a4e154`.
- **Frost em tudo que parece vidro**: o menu rápido e o painel de guia parental passam pelo material
  compartilhado, e o frost volta a usar a arte do título pré-desfocada em vez de sumir no meio do
  crossfade do destaque. Todas as plataformas. `46e0bd5bd`, `3982ef84b`.
- **Tela preta no lançamento** (#558): o teclado e o microfone opcionais da Samsung saem do caminho de
  inicialização e a falha aparece na tela em vez de travar. Samsung .tpk. `cc49341e6`, `a1888d007`.
- **URL direta toca sem P2P** (#552 #505): uma fonte com URL http(s) não desaparece mais por causa do
  `notWebReady` do add-on, e torrent continua pedindo o motor P2P (PR #559). Todas as plataformas.
  `d25a3e513`, `6d524f947`.
- **Hostnames e P2P** (#552): comparação de host sem diferenciar maiúsculas, ignora usuário e porta e entende
  loopback IPv4/IPv6, então fonte local não some. Todas as plataformas. `cbcf467f5`.
- **Muitos catálogos Jellyfin aparecem** (#468): servidores como AIOMetadata expõem uma visão por catálogo;
  o app parava em 6 e só aceitava movies/tvshows, então cerca de quatro catálogos apareciam. O teto subiu
  para 16 visões, com boxsets, mistos e sem tipo aceitos, e o orçamento de itens é dividido entre as linhas
  para a memória não crescer. Todas as plataformas. `b0e429762`, `c47a40788`.
- **Temporada especial do Kitsu sem quebrar a fonte**: quando o add-on Kitsu não informa a temporada, ela
  continua ausente em vez de virar 0 — a numeração do provedor segue valendo na fonte, na retomada e no
  próximo episódio. Todas as plataformas. `4a443422b`.
- **Perfil que não carregava o catálogo** (#551): sessão de perfil velha é recuperada antes de tentar a
  descoberta de novo, e um PIN que completa a escolha abre a sessão na hora. LG webOS. `1c9043ed0`, `a2e9daf91`.
- **Escolha explícita não se perde** (#545): fileira colocada à mão ou na fila continua valendo quando outras
  ficam fora por cota. Todas as plataformas. `21d1783b6`, `67985ea5d`.
- **Trailer em tela cheia com falha**: um trailer dublado que falha ou trava cai para Apple/IMDb em vez de
  ficar preto. Todas as plataformas. `5f4caef1c`, `1797b5306`, `1c7455e37`, `a6472f80d`.
- **Player externo não rouba a legenda** (#501): enquanto o "Perguntar" está aberto, um segundo toque em
  Tentar não começa a tocar por dentro. Android. `a04f2a820`.
- **Idioma original do trailer**: a escolha do idioma é respeitada no detalhe e na Home, e o candidato dublado
  correto é priorizado. Todas as plataformas. `c831a592f`, `c0ea07466`.
- **Vidro muito pesado**: o desfoque de fundo passou a ser uma captura por quadro (Android e .tpk também),
  em vez de 4 cópias e 20 passes por animação; some com o vapor de modal no meio. Todas as plataformas.
  `d98d72026`, `0c77460c1`, `5536ee8da`, `f8bc42b11`.
- **Pendrive com sistema de arquivos desconhecido**: o P2P falha fechado em vez de aceitar o volume, e o
  limite ativo é preservado. Todas as plataformas. `3b025a959`.
- **Busca abrir o título certo**: o título vem da seleção atual, não do último cartão desenhado, e a falha de
  inicialização do teclado Samsung aparece na interface. Todas as plataformas. `a2fd7e41d`, `cc49341e6`.
- **Ajustes explicam o P2P desligado**: antes de falar de cache USB, os Ajustes dizem por que o P2P está
  desligado, e porque Cache do P2P e Limite de espaço estão cinza. Todas as plataformas. `2e480806c`,
  `f170d77ba`, `6d537cb92`.
- **Comentários do Trakt voltam a aparecer nas séries** (pedido do dono): abrir uma série depois de ver um
  episódio não deixava mais a lista de comentários presa no modo episódio, e a leitura da resposta do add-on
  não quebrava em array vazio ou com espaço. Todas as plataformas. Refs pedido do dono, `5195cd14f`,
  `335bd2abc`.
- **Trakt comments show on series again** (owner request): opening a series after watching an episode no
  longer leaves the comment list stuck in episode mode, and the add-on response parser no longer breaks on an
  empty or space-prefixed array. All platforms. Refs owner request, `5195cd14f`, `335bd2abc`.
- **Log enviado com o começo**: o cabeçalho de inicialização (16 KB) volta a entrar no log enviado, que é o
  que serve para diagnosticar. Todas as plataformas. `84c1a90b6`, `3b12a5d2f`.

### Desempenho

- **Frost só nos retângulos que os painéis leem**: com o fundo Dinâmico adiado, a cópia do backdrop
  passou a ser resolvida uma vez por mudança de hash em vez de por quadro. Todas as plataformas.
  `5528dc0ad`.
- **Home e vidro muito mais leves no Android (TCL) e no .tpk**: uma captura de fundo por quadro em vez de
  4 cópias e 20 passes de desfoque (antes 4.00 capturas / 20 passes, agora 1.15 / 5.75).
  Android, Samsung .tpk. `0c77460c1`, `d98d72026`, `5536ee8da`, `d7ec89975`.
- **Continuar assistindo não trava a Home**: os catálogos abrem enquanto a fileira é montada.
  Todas as plataformas. PR #538, `cee137de7`, `cfc15aeab`, `8c99638f7`.
- **Buffer preso na memória ativa**: a faixa DTS em uso reserva seu 4 MiB antes de medir a fila.
  Todas as plataformas. `62421bfdc`.
- **Vidro fosco: desfoque regional e 3 passos em vez de 5**: so os retangulos que os paineis leem sao
  desfocados, o alvo para em 960x540 (480x270 em um passo) e cada alvo e limpo antes do passe; o corte de cena
  recaptura no mesmo quadro em vez de esperar. Home animada/foco da C9 a 1080p: 498.960 → 24.289 pixels de
  desfoque por quadro; a 4K 1.995.840 → 56.621. Custo extra: uma textura RGB cheia guardada por camada
  (5,93 MiB a 1080p). Todas as plataformas. `93b3d69ec`, `3dc70083c`, `a0f765d7f`, `17717812d`, `013e1341f`,
  `fe6a04804`, `4ae3e7211`.
- **Interior do vidro sem conta de borda**: a maior parte de um painel grande voltava depois de calcular a
  normal da borda e os pesos do aro que nunca usa; o caminho fosco resolve a densidade e sai antes. Mesma
  imagem. Todas as plataformas. `4dba47b60`.
- **Vidro fosco com arte pre-desfocada** — *revertido nesta janela*: o desfoque do vidro passou a ler uma copia
  em meia resolucao so da camada de fundo (1,15 → 0,35 capturas por quadro), mas o dono achou o resultado
  anterior mais bonito e mais leve na TCL e o merge foi desfeito (`f40a93dc3`); no lugar entrou a otimizacao
  do caminho atual (`4c82273d7`). Todas as plataformas. `e9c809475`, `ab02d4aaa` (caminho que saiu).
- **Home reage a reordenar/ativar fileiras sem recarregar tudo**: a atualização da rede depois de fechar
  Ajustes roda uma vez, e as linhas de servidor acima do teto da Home são cortadas em vez de descartadas.
  Todas as plataformas. `b0e429762`.
- **Fileira de comentários do Trakt não derruba mais o FPS** (pedido do dono): a quebra de linha do texto de
  cada cartão era recalculada palavra por palavra em todos os quadros (702 chamadas de layout por quadro
  rolando a fileira); agora usa o cache de quebra de linha compartilhado — 0 layouts e 0 uploads de textura
  por quadro, com a mesma aparência. Todas as plataformas. `1165245fc`, `a363d821c`, `335bd2abc`.
- **The Trakt comments row no longer costs FPS** (owner request): per-card text wrapping was recomputed
  word by word every frame (702 layout calls per frame while scrolling the row); it now uses the shared wrap
  cache — 0 layout calls and 0 texture uploads per frame, same look. All platforms. `1165245fc`, `a363d821c`,
  `335bd2abc`.
- **Detalhe: menos camadas de tela cheia**: furo e véu do cartão de trailer passam a ser um único desenho, e
  um cartão vizinho quase fora da tela deixa de contar como cheio — 4 → 2 camadas, preenchimento 1,81 → 0,99
  no harness, PNGs idênticos. Todas as plataformas. `5ed6ddfaa`.

### Comunidade

- **Modo TV e Celular automático no Android** — @community, PR #431 (`c305e6f7e`, `875d7f18f`).
- **Identidade da série na busca e na reprodução** — PR #561 (`cdf5bdd88`, `2537db6a0`, `86d194462`).
- **Idiomas de áudio e legenda informados pelo add-on** — @ghbarker, PR #487 (`e4fb8c08`; revisado, ainda não
  integrado — ver *Pendente*).
- **Continuar assistindo sem travar os catálogos** — @ghbarker, PR #538 (`cfc15aeab`, `a748ebf32`).
- **Retratos do elenco do add-on quando o TMDB falha** — @ghbarker, PR #540 (`a1eca619b`, `8cc5d723d`).
- **Tirar linhas de add-on desinstalado** — @ghbarker, PR #543 (`11e6e8535`, `0d44804f0`).
- **Backdrop completo e qualidade da arte respeitada** — @ghbarker, PR #546 (`946ad1819`, `66299e5f0`).
- **Relógio e ilha de retomada com Up** — @ghbarker, PR #547 (`a7d8b1f58`, `960ec872a`).
- **Temporada 0 (especiais) mantida e tocável** — @ghbarker, PR #569 (`8dff0e2a9`, `054ccef24`).
- **Lembrar a faixa de áudio escolhida** — @Tolu-Walop-E, PR #550 (`0c2a1ba2c`, `2cec223fd`).
- **URL direta toca sem P2P** — @mark-major, PR #559 (`d25a3e513`, `06724b3ed`).

### Interno

- Testes novos: `tests/dvabertura_shot.sh` (FAIL antes, PASS depois) para a abertura DV; fileiras com
  os 3 tamanhos numa linha, enum acrescentado no fim e ajuda no catálogo e na coleção; geometria monotônica
  do cartaz grande nos dois layouts; PNGs dos três tamanhos no `pgtam_shot` (`e50a38ef6`, `331746034`).
- Teste novo do pisca do fundo Dinâmico (`tests/dina_pisca_shot.sh`) e do hero dividido (últimos itens da
  fileira, sem quebra) (`5528dc0ad`, `4428dea18`).
- Vidro defumado: o teste passa a exigir zero capturas e zero passes de desfoque e a contar os vidros
  defumados; a fonte de desfoque pré-assada do player foi removida (`6f1c61cb8`).
- Testes cobrindo Continuar assistindo independente, inicialização de catálogos, remoção de add-on,
  retratos de metadados, modo automático Android, limites do player, relógio e propriedade de registro
  (`8c99638f7`, `bd6e61119`, `a06447082`, `8979ef16c`, `d570cb3c1`, `85562a66b`, `5f0ad732e`, `5ae92423e`,
  `bbb74484e`).
- Varredura de tradução limpa: motivos de log em inglês para amigos, fileiras e USB
  (`171e49772`, `6375a1520`, `35efbf90a`).
- Especificação de Dolby Vision perfil 7 → 8.1 (documento, não implementado) e preparo do pendrive
  (`4907caa50`, `db03a2bc5`).
- `repo.json` do homebrew alinhado com a 2.0.6 (`c2e50a932`).

- Deep links: parser, fila limitada e cobertura de regressão (`8f38bb943`, `7cf145dca`, `f44e64863`).
- Simkl anime mesclado às listas de séries com dedupe por imdb (`f5dab2f8a`).
- Vidro: captura única do backdrop por mudança de hash (`e9c809475`) e harness NV_FPS para o detalhe
  (`9356bc16c`).
- Testes do cabeçalho de coleção fora da suíte padrão, mocks em `*_shot` (`eb03a56a4`, `1c3043479`) e fixtures
  de sessão/perfil para Continuar assistindo e catálogos (`8d8f5c92a`, `8c99638f7`, `840dacb67`).
- Stubs de teste para o merge do master: fileiras (`c76ed4632`), home (`5a926a863`) e fontes/idiomas
  (`33b0d7de0`).
- Triagem dos PRs abertos (`72ba10761`) com o port de fixes de busca, coleção, Trakt e player
  (`e0ddeca5f`, `4baf783fa`, `ff1f1aa25`, `61d73a5bf`).
- Teste novo `tests/homerefresh.sh`: a Home segue reordenar/desativar fileiras e novas fontes sem reiniciar;
  fixtures de home/coleção consertadas depois do merge (`c47a40788`, `0bfe5ce71`).
- Limite de diferenca de pixel do frost a 1080p fixado em teste: os 12 PNGs da C9 contra o caminho antigo,
  mascarando so os digitos do relogio (`4ae3e7211`); testes de identidade de serie titulada e de toque tardio
  (`2537db6a0`); assert de que o estilo 5 da fileira de amigos nao tem coluna de "+ Adicionar"
  (`d95bd32e1`) e de que o scroll de toque inclui o painel do ultimo amigo (`e27514693`).
- Migracao da escala dos Ajustes para 100% de fabrica so para quem nunca escolheu na mao
  (`3f8d304b7`) e prova de pixel de que as abas da Biblioteca/catalogo tem frost regional
  (`21696fd8c`, `796a3441f`).
- Fixture do `tests/homerefresh.sh` consertada outra vez no merge: stubs duplicados de home e stubs de
  trava de add-on que faltavam (`c47a40788`).
- Testes novos dos comentários do Trakt (`tests/traktcoment.sh`, dados de série/episódio, contador de 180
  quadros e FAIL->PASS da regressão de série), do cache de quebra de linha da fileira e do token de geração
  da folha de fontes / "Assistir do começo" (`5195cd14f`, `1165245fc`, `075fec9a4`, `6a6d5b71b`).

### Pendente (ainda não integrado)

Trabalho em branch aberta, sem merge — **não** faz parte da 2.0.7 como está:

- **PR #487 v2** (idiomas de áudio e legenda informados pelo add-on) — `agente/205-pr487v2`, 3 commits fora,
  VEREDITO OK `j-rev-pr487-v2`, sem commit depois. **Needs final re-review.** Android, LG webOS.
- **PR #493 v2 / v3** (perfil e sessão) — `agente/205-pr493`, 12 commits fora. VEREDITO **BLOQUEIA**
  `j-revg-pr493` (mais novo); `j-revpr493-v3` OK foi antes. Precisa de fix novo + review.
  Android, LG webOS.
- **Menu "mais" do detalhe no vidro** — `agente/205-detalheglass`, 2 commits fora (`e047eecee`, `9ebdc9a3b`):
  elenco, recomendações, trailers e o menu "mais" da página do título passam a usar o vidro/frost
  compartilhado. Fix novo (`0b52916d0`, `d04f21ba4`, 06:01) e **review rev2 BLOQUEIA (06:51)**: o vidro
  ainda esconde a arte da coleção carregada (P1) e o teste do menu continua aceitando a implementação
  antiga (P2). Precisa de fix novo + review antes de entrar na 2.0.7. Todas as plataformas.
- **Frost da barra lateral e pílula de acento em vidro** — `agente/205-vidromenu` foi **merged**
  (`ab0e9a597`, 08:11, conflitos resolvidos pelo merge) e `agente/205-vidroilhas` foi **merged**
  (`f087f4b59`); os dois já estão nos Destaques. O review rev2 OK (`j-rev2-vidromenu`, 06:49) foi **antes**
  do merge, então falta revisar o resultado mesclado. Todas as plataformas.
- **Fundo Cinza/Cor fixo no layout Dinâmico** — `agente/205-dinfundo` foi **merged** (`e3fdbe717`, 07:49);
  review rev2 OK (06:48) depois do fix da paleta de acento (`0cf805980`). Já está nos Destaques.
- **Escala de texto e componentes** — `agente/205-textoescala`, 5 commits fora (`e7fa6d67c`,
  `406359d3a`, `bab9c56e3`, `86fed77d7`, `4bfafc814`): ajuste novo "escala de texto e componentes"
  (85–130%) que multiplica texto, botões e componentes sem mexer no tamanho dos cartazes. **Review
  BLOQUEIA (06:50)**: a opção não aparece em Ajustes › Aparência (P1), os cartazes continuam
  redimensionando (P2) e as linhas da Biblioteca se sobrepõem a 85% (P2). Pedido do dono. Todas as
  plataformas.
- **Frost barato igual ao aprovado na LG C9** — `agente/205-foscovelho` foi **merged** (`76bc9d0c3`) e
  depois **revertido** (`f40a93dc3`): na TCL o dono achou o fosco anterior (captura de tela + desfoque) mais
  bonito e mais leve. A fonte de arte pré-desfocada (`ab02d4aaa`, `d0789c13a`, `50a1ac232`, `35f5f4fc3`)
  **não** está na 2.0.7; o review r4 deu OK (`j-rev-foscovelho-r4`, 22:08) já depois do merge. O caminho que
  ficou — a captura de tela, só que barata — entrou por `4c82273d7` e está em Destaques/Desempenho.
  Todas as plataformas.
- **Comentários do Trakt com vidro, menu contextual com frost e #562/PR #574** entraram nesta janela
  (`335bd2abc`, `dc040ef2d`, `2c7c85860`, todos com review OK: `rev-traktcoment`, `rev-ctxgrande`,
  `rev-pr574`) e já estão em Novidades/Correções/Desempenho. Falta medir o FPS dos 8 painéis de vidro em TV.
- **Frost otimizado, fileira de amigos polida, PR #561 (#524), abas com frost e Ajustes em 100%** entraram
  nesta janela (`4c82273d7`, `f21d993b2`, `86d194462`, `a096e5eb8`, `f22761e28`) e já estão em
  Destaques/Novidades:
- **Vidaa** — `agente/205-vidaa` foi **merged** (`d0bf5ba79`, 10-15 22:10); VEREDITO OK `j-revg-vidaa-6`
  depois de uma sequência de BLOQUEIA. Sai na 2.0.7; não está em pendência.
- **Cabeçalho de coleção diferente** (pedido do dono) — `agente/205-colhdr` foi **merged** (`fec2dc0ac`),
  review OK `j-revg-colhdr`; não está em pendência.
- **Tizen/homebrew, glassmock, mock21, perfis21, respostas ao dono** — branches com job feito e nenhum review
  (`agente/205-homebrew`, `agente/205-tizenbrew`, `agente/205-glassmock`, `agente/205-mock21`,
  `agente/205-perfis21`, `agente/205-respostas`). Sem conteúdo de usuário confirmado.
- **Fonte de legenda customizada** — a segunda metade de #501, sem branch. Android.
- **Sync manual de legenda** — `agente/205-legajuste`, só especificação. Adiado para 2.1.
- **PRs abertos sem branch local**: #456/#457/#466/#467/#468/#469/#478/#482 (os de fora de escopo ficam no
  mapa). Triagem, não perdidos.

---

## EN (same list, English copy)

### Highlights

- **Deep links** (#539): `nuvio-legacy://`, `stremio://` and IMDb links open the app straight to a title,
  to search, or already playing, with the newest link kept queued while discovery runs. All platforms.
  Refs #539, `8f38bb943`, `a3eb1e19a`, `7cf145dca`, `f44e64863`, `a4f220cd7`.
- **P2P cache on a USB drive, on every platform** (owner requests, #486 #505): USB caching now shows up and
  works on Android and Samsung .tpk, not just webOS, with the size limit and the reason for each rejection
  spelled out. Android, Samsung .tpk. Refs owner requests, `31a62b273`, `51daddd84`, `b090905dc`, `a633888a3`,
  `ebb5eb8fe`, `9b4f37efb`.
- **External player on Android TV** (#501): pick which installed video app opens the source in
  *Settings › Player* (Internal, External, Ask every time) — plus a separate choice for anime. Subtitle and
  aspect preferences survive. Android. Refs #501, `f954a272c`, `276207fac`, `059f4d4f3`.
- **Automatic TV and Mobile mode in the standard Android APK** (PR #431): the app picks the interface from
  screen size and input, nothing to switch by hand. Android. Refs PR #431, `c305e6f7e`, `8979ef16c`, `875d7f18f`.
- **Subtitle outline reads solid** (owner requests): the subtitle outline is now an opaque 8-direction ring
  instead of a translucent drop shadow, and the chosen opacity applies once instead of compounding at the
  corners. LG webOS, Samsung .tpk. Refs `6a76a46aa`, `d0f6f5986`, `f29e252c2`.
- **Simkl with anime** (owner requests): Simkl's third type (anime) joins the series lists, Continue Watching
  and Plan to Watch, deduplicated. All platforms. Refs `f5dab2f8a`, `d6244a1f1`.
- **Specials (season 0) are shown and playable** (#556, PR #569): anime with a special or out-of-order season
  that Trakt recognises are no longer dropped; the provider's episode numbering survives through the source,
  resume, next episode and Trakt/Simkl/Plex, and the fake per-episode season tab an add-on creates ("The
  Blacklist: Season 20+") is gone. All platforms. Refs #556, PR #569, `8dff0e2a9`, `4a443422b`, `94d16f7de`,
  `288e531ee`, `054ccef24`.
- **Frosted glass is cheaper and looks the same** (owner request): the glass blur now works only on the
  rects the panels read and at most at 960x540, with 3 passes instead of 5 and no stale recapture after a hard
  cut. On the C9 Home at 1080p the blur went from 498,960 to 24,289 pixels per frame and all twelve reference
  shots stay within 4 levels. All platforms. Refs owner request, `4c82273d7`, `93b3d69ec`, `3dc70083c`,
  `013e1341f`, `fe6a04804`, `4dba47b60`.
- **Polished friends row, no "See all"/"+ Add"** (owner request): all six friends row styles now use the
  standard Home focus (no red outline), landscape posters with text in the right place, an initial-letter
  avatar when there is no photo, a fallback poster when there is no art, and the two row buttons are gone.
  All platforms. Refs owner request, `8806e6aff`, `d95bd32e1`, `e27514693`, `f21d993b2`.
- **Mark as watched works** (#548): marking a whole series or one episode sticks after an older whole-series
  job or retry answers later, and confirmed marks survive a restart and a stale Trakt pull. All platforms.
  Refs #548, `66a608ef7`, `8330673a8`, `2d04fbecc`, `b074d9fd5`, `91da5935e`.
- **Player opening screen** (owner request): the logo slides to the centre and the Play pill morphs into a thin
  progress ring below it, over sharp artwork with a dark veil rising to half the screen; "Arte" is the new
  default, with "Arte borrada" and "Desligado" kept. The new **Estilo** setting picks **Centro (A3)** or
  **Canto (B3)**, logo bottom-left with a thin ring under it. All platforms. Refs owner request,
  `718e61abb`, `ace1fd53a`, `922500ef4`, `9c3ecdf85`.
- **Fixed Gray or Colour background in the Dynamic layout** (owner request): the Dynamic ("Apple TV")
  layout background is no longer art-only — Gray and Colour are now options, picked with the accent palette
  picker the settings already used, with a calmer Immersive mode. All platforms.
  Refs owner request, `6da1fc418`, `e91329dd2`, `0cf805980`, `e3fdbe717`.
- **Sidebar frost and Home frost islands** (owner request): the sidebar frosts over screens that carry
  glass, the accent pill becomes tinted glass in glass mode, the frost islands are no longer drawn under the
  Home sidebar glass, and modals only capture the region the panel reads — including the "Saved" modal.
  All platforms. Refs owner request, `90b0f9cd7`, `61764f975`, `fb4af3ee5`, `f087f4b59`, `7bf4caac9`,
  `e3eb56d39`, `37687dce0`, `90c8d380d`, `ab0e9a597`.
- **Sources sheet grows from the clock**: the sources sheet now opens out of the clock island, with the same
  spring and material as the rest of the interface. All platforms. `fbe06f5e7`, `ae39b748a`.
- **See all grid** (PR #568 #563): showcase rows in collections and the search results of each catalog open
  into a grid of every item, without the row limit. All platforms. Refs PR #568 #563, `4baf783fa`,
  `e0ddeca5f`.
- **Catalog outside the quota refreshes right away** (#545 follow-up): a row you just added renews on its own,
  without needing another action. All platforms. `975638744`, `53b050753`, `b03502f84`.
- **Search and episodes work again** (#555): exact-title series like "Bigg Boss" are no longer cut by the
  result cap, and series that come from an add-on with no episodes get their list filled from the catalog.
  All platforms. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catalogs: ghost rows and vanishing sources** (#545 #551 #552): rows of uninstalled add-ons leave Home, rows
  the add-on does not declare this session no longer take a slot, and sources come back when the add-on probe
  is slow. All platforms. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`, `455135df2`, `cbcf467f5`, `27988615e`.

### New

- **Row title styles**: *Settings › Home › Row titles* picks Classic, Island (title in a glass pill with
  the row icon and item count), Editorial (small uppercase kicker above the spaced title) or Accent (a dot
  that grows into a bar on the focused row, with a "See all" tail). All platforms.
  Refs owner request, `331746034`, `4898972d7`.
- **Big poster row in three sizes**: the info-on-poster row gains Small, Medium and Large
  (*Settings › Home appearance › Row style*, left/right like Landscape and 4:3) — Small with one synopsis
  line, Large with three and the spare height turned into width. Medium is unchanged.
  All platforms. Refs owner request, `e50a38ef6`, `cbf6fb021`.
- **Smoked glass over the video**: in the player and the panels drawn over live video the glass stops
  blurring and becomes tinted translucent ink, with no screen capture. All platforms.
  Refs owner request, `6f1c61cb8`, `a691e96fb`.
- **Automatic glass quality**: *Settings › Glass quality* (Automatic, Performance, High) — Performance uses
  only the smoked glass, with no capture or blur; Automatic drops to smoked by itself after 4 consecutive
  150-frame windows over the target frame time. All platforms.
  Refs owner request, `30a932909`, `a691e96fb`.
- **Dolby Vision opening in the new style**: the Dolby Vision Cinema screen now uses the same opening as the
  other loading screens, with Style (Centre/Corner), Background (Off/Art/Blurred), the add-on line, the
  detailed card and reduced animations all applying the same way. All platforms.
  Refs owner request, `d89784e82`, `8f788b25b`, `b59987511`, `ef33525a8`, `4542aeffe`, `abb9e49b9`.
- **Frost in the player and its menus**: the buttons, the control bar, the source/subtitle/audio panels,
  the live Statistics panel and the quick menu / parental guide now use the same frosted glass as the rest
  of the interface instead of a flat panel. All platforms (where the video can be read by GL).
  Refs owner requests, `46e0bd5bd`, `3982ef84b`.
- **Translucent top chrome with frost off**: with Glass on and "Vidro fosco" off, the top bar (and the
  avatar/clock island) is see-through tinted glass instead of opaque. LG webOS. `b5007ae85`.
- **Home follows your changes without a restart** (#468, owner request): reordering or disabling rows in
  Settings and adding a new Jellyfin server/add-on refreshes Home on the spot, and a server with many
  libraries stops showing only four catalogs — the view cap went from 6 to 16, boxsets and untyped views
  are accepted. All platforms. Refs #468, `b0e429762`, `c47a40788`, `08c98b017`.
- **One less button in the player** (owner request): the previous-episode button leaves the series action
  bar; next episode and every other button stay, with focus order and spacing intact. All platforms.
  Refs owner request, `4e6b592d8`, `d51d4e46c`.
- **"P2P cache" row on Android**: the USB drive cache setting shows up and works in the standard Android
  APK, as it already did on webOS. Android. `9b4f37efb`.
- **See all grid in collections**: showcase rows open into a full "See all" grid of the collection.
  All platforms. `4baf783fa`.
- **Exact episode identity in scrobbles**: progress sent to Trakt uses the exact episode identity.
  All platforms. Refs PR #560, `ff1f1aa25`.
- **Series identity kept through lookup and playback** (#524, PR #561): a series stays the same series while it
  is looked up and played, instead of turning into a different title, and the requested episode is the one that
  plays — this covers Continue Watching and the resume cards too. All platforms. Refs #524, PR #561,
  `cdf5bdd88`, `2537db6a0`, `86d194462`.
- **Continue Watching up to 40 items** (PR #538): the native cap went from 12 to 40 and future rows are
  published up to it. All platforms. Refs PR #538, `4cbf5ad5c`, `696a3a6ee`, `cee137de7`.
- **Buffer size** (*Settings › Buffer size*): Automatic, 64, 128 or 256 MiB, always clamped to free RAM.
  All platforms. `42b3718cf`, `62421bfdc`, `35a944d9f`.
- **Split hero row style** (*Settings › Home appearance › Row style*): the focused title becomes a large
  landscape panel with art and synopsis. All platforms. `8655956cb`, `669fa7b5c`, `f8815746a`, `76c6e8eea`.
- **Trailer audio**: *Trailer audio* picks Automatic, Original or any language in the list, and prefers the
  dubbed trailer when one exists. All platforms. `b43655122`, `c831a592f`, `c0ea07466`, `31dcf5275`.
- **Captions in the YouTube trailer** (webOS): the embedded trailer follows the subtitle language.
  LG webOS (wasm). `e23f0476d`.
- **Continue Watching up to 40 items**: the row publishes everything that fits instead of stopping at 12.
  All platforms. PR #538, `4cbf5ad5c`, `696a3a6ee`, `cee137de7`.
- **Animated row style picker**: opening and closing the picker animates smoothly, and a stray tap during
  the close no longer changes anything. All platforms. `92656f2ae`, `84239e65a`, `40b7d8e15`, `86c763a2d`.
- **Clock with Up**: from browsing pages, Up reaches the clock and the resume island (PR #547).
  All platforms. `a7d8b1f58`, `960ec872a`.
- **Reorder add-on blocks on mobile** and **save feedback in place**: Settings in the APK's Mobile mode
  explain what only exists on TV and confirm the save. Android. `4bd7a33cc`, `acc47411f`, `29a516106`,
  `7163e053c`, `510007bdd`.
- **More reliable cast and metadata**: add-on portraits stay when TMDB fails, accented and emoji names no
  longer duplicate, and shallow recommendation backdrops are upgraded at High quality. All platforms.
  PR #540 PR #546, `773c576f7`, `a0e225043`, `946ad1819`.
- **Audio track remembered**: the hand-picked track sticks for the whole show, including after closing the
  app (PR #550). All platforms. `0c2a1ba2c`.
- **English friends log**: the friends row reason no longer dirties the translation scan. All platforms.
  `171e49772`.

- **Library and catalog tabs are real glass** (owner request): the tab strips (Saved/Collection/Lists,
  Movies/Series, the title pill and the remaining segmented controls) now use the same frosted glass as the
  rest of the app, with the selected tab keeping its accent fill. All platforms. Refs owner request,
  `21696fd8c`, `796a3441f`, `a096e5eb8`.
- **Settings at 100% by default** (owner request): the Settings scale ships at 100% instead of 80%; anyone who
  never picked a size by hand moves to 100% and an explicit choice stays as it was. All platforms.
  `3f8d304b7`, `f22761e28`.
- **High-quality artwork is fetched at the size it needs** (owner requests): at High quality, w342 posters and
  w780 backdrops stored by the catalog are fetched at the size the draw needs, and the 480px small backdrop
  is no longer served for cards up to 640px. All platforms. `ff1cd6dcd`, `44bf4884a`.
- **Long poster URL templates** (owner requests): pasting a long poster URL template on the phone page is no
  longer cut at 1024 characters, and the native keyboards accept the same limit. All platforms. `0950a8eb8`,
  `df7c93aaf`.
- **MKV audio track remembered**: on MKV with a converted track, the hand pick keeps sticking for the next
  episode, even with two identical tracks. All platforms. `3486c616b`.

### Fixes

- **Dynamic background no longer flickers**: with frosted glass on in the Dinâmica layout the deferred
  background is resolved before the frost backdrop copy, so Home stops alternating between the right art
  and colour bands on Android. Android. Refs owner request, `5528dc0ad`, `6a64c2e13`.
- **Split hero without a repeated title**: in the split hero the poster list no longer repeats the title
  that is already in focus. All platforms. Refs owner request, `4428dea18`, `f41a4e154`.
- **Frost on everything that looks like glass**: the quick menu and the parental guide panel now go
  through the shared frosted material, and the frost source survives the hero crossfade (baked title art)
  instead of dropping to the opaque fallback. All platforms. `46e0bd5bd`, `3982ef84b`.
- **Black screen on launch** (#558): optional Samsung keyboard and microphone APIs leave the launch path and
  the failure is surfaced instead of hanging. Samsung .tpk. `cc49341e6`, `a1888d007`.
- **A direct URL plays without P2P** (#552 #505): a source with an http(s) URL no longer disappears because
  of the add-on's `notWebReady`, while torrents still ask for the P2P engine (PR #559). All platforms.
  `d25a3e513`, `6d524f947`.
- **Hostnames and P2P** (#552): case-insensitive full-hostname comparison, userinfo and ports skipped,
  numeric IPv4/IPv6 loopback recognised, so local sources stop vanishing. All platforms. `cbcf467f5`.
- **Profile that never loaded the catalog** (#551): a stale profile session is recovered before retrying
  discovery, and a PIN that completes the choice opens the session immediately. LG webOS. `1c9043ed0`, `a2e9daf91`.
- **Explicit choice no longer lost** (#545): a row added by hand or placed in the queue stays eligible while
  others stay out on quota. All platforms. `21d1783b6`, `67985ea5d`.
- **Fullscreen trailer with a failure**: a dubbed YouTube trailer that fails or stalls falls back to
  Apple/IMDb instead of staying black. All platforms. `5f4caef1c`, `1797b5306`, `1c7455e37`, `a6472f80d`.
- **External player no longer steals the subtitle** (#501): while the "Ask" modal is open, a second Tentar
  tap does not start internal playback. Android. `a04f2a820`.
- **Original trailer language**: the language choice is honoured on the detail page and Home, and the right
  dubbed candidate is ranked first. All platforms. `c831a592f`, `c0ea07466`.
- **Frosted glass far cheaper**: one backdrop capture per frame on Android and .tpk too, instead of 4 copies
  and 20 blur passes per animation; the shared capture is dropped when a modal veil intervenes.
  All platforms. `d98d72026`, `0c77460c1`, `5536ee8da`, `f8bc42b11`.
- **USB drive with an unknown filesystem**: P2P fails closed instead of accepting the volume, and the active
  limit is retained. All platforms. `3b025a959`.
- **Search opens the right title**: identity comes from the current selection instead of the last drawn card,
  and Samsung keyboard startup failure is reported in the UI. All platforms. `a2fd7e41d`, `cc49341e6`.
- **Settings explain a disabled P2P**: before USB cache advice, Settings say why P2P is off, and why P2P cache
  and Space limit are greyed out. All platforms. `2e480806c`, `f170d77ba`, `6d537cb92`.
- **Uploaded log keeps its start**: the 16 KB startup header is back in the uploaded log, which is the part
  that diagnoses. All platforms. `84c1a90b6`, `3b12a5d2f`.
- **Many Jellyfin catalogs show up** (#468): servers like AIOMetadata expose one view per catalog; the app
  stopped at 6 and only accepted movies/tvshows, so about four catalogs appeared. The cap went to 16 views,
  with boxsets, mixed and untyped views accepted, and the item budget is split across rows so memory does
  not grow. All platforms. `b0e429762`, `c47a40788`.
- **Kitsu special season without breaking the source**: when the Kitsu add-on does not report a season it
  stays absent instead of becoming 0 — the provider's numbering keeps working in the source, on resume and on
  the next episode. All platforms. `4a443422b`.

### Performance

- **Frost only on the rects the panels read**: with the Dynamic background deferred, the backdrop copy is
  resolved once per hash change instead of per frame. All platforms. `5528dc0ad`.
- **Much lighter Home and glass on Android (TCL) and .tpk**: one backdrop capture per frame instead of
  4 copies and 20 blur passes (4.00 captures / 20 passes before, 1.15 / 5.75 now). Android, Samsung .tpk.
  `0c77460c1`, `d98d72026`, `5536ee8da`, `d7ec89975`.
- **Continue Watching no longer blocks Home**: catalogs open while the row is built. All platforms.
  PR #538, `cee137de7`, `cfc15aeab`, `8c99638f7`.
- **Buffer clamp accounts for active DTS**: the in-use range reserves its 4 MiB before queue capacity is
  chosen. All platforms. `62421bfdc`.
- **Frosted glass: regional blur and 3 passes instead of 5**: only the rects the panels read are blurred, the
  target stops at 960x540 (480x270 in one pass) and each target is cleared before its pass; a hard cut recaptures
  on the same frame instead of waiting. Animated/focused C9 Home at 1080p: 498,960 → 24,289 blurred pixels per
  frame; at 4K 1,995,840 → 56,621. Extra cost: one full RGB texture saved per layer (5.93 MiB at 1080p).
  All platforms. `93b3d69ec`, `3dc70083c`, `a0f765d7f`, `17717812d`, `013e1341f`, `fe6a04804`, `4ae3e7211`.
- **Glass interior skips the rim math**: most of a big panel used to return after computing the edge normal and
  rim weights it never uses; the frosted path solves density and returns first. Same output. All platforms.
  `4dba47b60`.
- **Frosted glass from pre-blurred art** — *reverted in this window*: the glass blur started reading a half-res
  copy of the backdrop layer alone (1.15 → 0.35 captures per frame), but on the TCL the owner found the previous
  look prettier and lighter and the merge was undone (`f40a93dc3`); the optimization of the current path landed
  instead (`4c82273d7`). All platforms. `e9c809475`, `ab02d4aaa` (the path that left).
- **Home reacts to reordering/enabling rows without reloading everything**: the network refresh that follows
  closing Settings runs once, and server rows over the Home cap are clamped instead of dropped. All platforms.
  `b0e429762`.
- **Detail: fewer full-screen layers**: the trailer card's hole and veil are drawn in one pass, and a
  neighbour card almost off screen no longer counts as full — 4 → 2 layers, fill 1.81 → 0.99 on the harness,
  PNGs byte-identical. All platforms. `5ed6ddfaa`.

### Community

- **Automatic TV and Mobile mode on Android** — @community, PR #431 (`c305e6f7e`, `875d7f18f`).
- **Series identity during lookup and playback** — PR #561 (`cdf5bdd88`, `2537db6a0`, `86d194462`).
- **Add-on reported audio and subtitle languages** — @ghbarker, PR #487 (`e4fb8c08`; reviewed, not merged —
  see *Pending*).
- **Continue Watching without blocking catalogs** — @ghbarker, PR #538 (`cfc15aeab`, `a748ebf32`).
- **Keep add-on cast portraits when TMDB fails** — @ghbarker, PR #540 (`a1eca619b`, `8cc5d723d`).
- **Remove uninstalled add-on rows** — @ghbarker, PR #543 (`11e6e8535`, `0d44804f0`).
- **Full recommendation backdrop and artwork quality** — @ghbarker, PR #546 (`946ad1819`, `66299e5f0`).
- **Reach the clock and resume island with Up** — @ghbarker, PR #547 (`a7d8b1f58`, `960ec872a`).
- **Season 0 Specials kept and playable** — @ghbarker, PR #569 (`8dff0e2a9`, `054ccef24`).
- **Remember the hand-picked audio track** — @Tolu-Walop-E, PR #550 (`0c2a1ba2c`, `2cec223fd`).
- **A direct URL plays without P2P** — @mark-major, PR #559 (`d25a3e513`, `06724b3ed`).

### Internal

- New tests: `tests/dvabertura_shot.sh` (FAIL before, PASS after) for the DV opening; rows with the three
  sizes on one line, the enum appended at the end and help text in both catalog and collection; monotonic
  geometry of the big poster in both layouts; PNGs of the three sizes in `pgtam_shot` (`e50a38ef6`,
  `331746034`).
- New tests for the Dynamic background flicker (`tests/dina_pisca_shot.sh`) and the split hero (last items
  of the row, no wrap) (`5528dc0ad`, `4428dea18`).
- Smoked glass: the test now requires zero captures and zero blur passes and counts the smoked glasses; the
  baked stand-in blur source in the player was dropped (`6f1c61cb8`).
- Tests covering independent Continue Watching, catalog startup, add-on removal, metadata portraits,
  automatic Android mode, player limits, the clock, and registry ownership (`8c99638f7`, `bd6e61119`,
  `a06447082`, `8979ef16c`, `d570cb3c1`, `85562a66b`, `5f0ad732e`, `5ae92423e`, `bbb74484e`).
- Clean translation scan: English log reasons for friends, rows and USB (`171e49772`, `6375a1520`, `35efbf90a`).
- Dolby Vision profile 7 → 8.1 conversion spec (document only, not implemented) and USB drive preparation
  spec (`4907caa50`, `db03a2bc5`).
- Homebrew `repo.json` aligned with 2.0.6 (`c2e50a932`).

- Deep links: parser, bounded queue and regression coverage (`8f38bb943`, `7cf145dca`, `f44e64863`).
- Simkl anime merged into the series lists, deduplicated by imdb (`f5dab2f8a`).
- Glass: single backdrop capture per hash change (`e9c809475`) plus an NV_FPS harness for the detail page
  (`9356bc16c`).
- Collection header mocks moved out of the default suite as `*_shot` (`eb03a56a4`, `1c3043479`), plus
  session/profile fixtures for Continue Watching and catalogs (`8d8f5c92a`, `8c99638f7`, `840dacb67`).
- Test stubs for the master merge: rows (`c76ed4632`), home (`5a926a863`) and sources/languages
  (`33b0d7de0`).
- Triaged open PRs (`72ba10761`) and ported fixes for search, collections, Trakt and player
  (`e0ddeca5f`, `4baf783fa`, `ff1f1aa25`, `61d73a5bf`).
- New `tests/homerefresh.sh`: Home follows row reorder/disable and new sources without a restart; home and
  collection fixtures repaired after a merge (`c47a40788`, `0bfe5ce71`).
- 1080p frost pixel-difference limit pinned in a test: all twelve C9 PNGs against the old path, masking only
  the live clock digits (`4ae3e7211`); typed-title and late-playback identity tests (`2537db6a0`); an assert
  that friends row style 5 has no "+ Add" column (`d95bd32e1`) and that its touch scroll covers the last
  friend's expanded panel (`e27514693`).
- Settings scale migration to a 100% factory default, only for anyone who never chose by hand
  (`3f8d304b7`), and a pixel proof that the Library/catalog tabs carry regional frost
  (`21696fd8c`, `796a3441f`).
- The `tests/homerefresh.sh` fixture repaired again at merge time: duplicate home stubs and missing add-on
  lock stubs (`c47a40788`).
- New Trakt-comment tests (`tests/traktcoment.sh`, series/episode fixtures, a 180-frame per-frame counter and
  the FAIL->PASS for the series regression), the shared wrap cache for that row and the sources-sheet
  generation token / "Play from beginning" (`5195cd14f`, `1165245fc`, `075fec9a4`, `6a6d5b71b`).

### Pending (not integrated yet)

Work on open branches with no merge — **not** part of 2.0.7 as it stands:

- **PR #487 v2** (add-on reported audio and subtitle languages) — `agente/205-pr487v2`, 3 commits out,
  VEREDITO OK `j-rev-pr487-v2`, no commit after. **Needs final re-review.** Android, LG webOS.
- **PR #493 v2 / v3** (profile and session) — `agente/205-pr493`, 12 commits out. VEREDITO **BLOQUEIA**
  `j-revg-pr493` (newer); the OK `j-revpr493-v3` came before. Needs a new fix + review.
  Android, LG webOS.
- **Cheap frost identical to the one approved on the LG C9** — `agente/205-foscovelho` was **merged**
  (`76bc9d0c3`) and then **reverted** (`f40a93dc3`): on the TCL the owner found the previous frosted glass
  (screen capture + blur) prettier and lighter. The pre-blurred art source (`ab02d4aaa`, `d0789c13a`,
  `50a1ac232`, `35f5f4fc3`) is **not** in 2.0.7; its round-4 review was OK (`j-rev-foscovelho-r4`, 22:08),
  already after the merge. The path that shipped — the screen capture, just cheaper — landed as `4c82273d7`
  and is under Highlights/Performance. All platforms.
- **Detail "more" menu on glass** — `agente/205-detalheglass`, 2 commits out (`e047eecee`, `9ebdc9a3b`):
  cast, recommendations, trailers and the title page "more" menu move to the shared glass/frost. New fix
  (`0b52916d0`, `d04f21ba4`, 06:01) and **rev2 review BLOQUEIA (06:51)**: the glass still hides loaded
  collection artwork (P1) and the menu test still accepts the old implementation (P2). Needs a new fix +
  review before 2.0.7. All platforms.
- **Sidebar frost and glass accent pill** — `agente/205-vidromenu` was **merged** (`ab0e9a597`, 08:11,
  conflicts resolved by the merge) and `agente/205-vidroilhas` was **merged** (`f087f4b59`); both are now
  under Highlights. The rev2 review was OK (`j-rev2-vidromenu`, 06:49) **before** the merge, so the merged
  result still needs a review. All platforms.
- **Fixed Gray/Colour background in the Dynamic layout** — `agente/205-dinfundo` was **merged**
  (`e3fdbe717`, 07:49); rev2 review OK (06:48) after the accent-palette fix (`0cf805980`). It is now under
  Highlights.
- **Text and component scale** — `agente/205-textoescala`, 5 commits out (`e7fa6d67c`, `406359d3a`,
  `bab9c56e3`, `86fed77d7`, `4bfafc814`): a new "text and component scale" setting (85–130%) that scales
  text, buttons and components while leaving poster/card sizes alone. **Review BLOQUEIA (06:50)**: the
  option never shows up in Settings › Appearance (P1), cards still resize (P2) and Library rows overlap at
  85% (P2). Owner request. All platforms.
- **Trakt comments with glass, frosted context menu and #562/PR #574** landed in this window
  (`335bd2abc`, `dc040ef2d`, `2c7c85860`, all review OK: `rev-traktcoment`, `rev-ctxgrande`, `rev-pr574`) and
  are already in New/Fixes/Performance. Still to measure on a device: the FPS of the 8 glass panels.
- **Cheaper frost, polished friends row, PR #561 (#524), glass tabs and Settings at 100%** landed in this
  window (`4c82273d7`, `f21d993b2`, `86d194462`, `a096e5eb8`, `f22761e28`) and are already under
  Highlights/New:
- **Vidaa** — `agente/205-vidaa` was **merged** (`d0bf5ba79`, 10-15 22:10); VEREDITO OK `j-revg-vidaa-6`
  after a run of BLOQUEIAS. It ships in 2.0.7; no longer pending.
- **Different collection header** (owner request) — `agente/205-colhdr` was **merged** (`fec2dc0ac`),
  review OK `j-revg-colhdr`; no longer pending.
- **Tizen/homebrew, glassmock, mock21, perfis21, owner answers** — branches with a finished job and no review
  (`agente/205-homebrew`, `agente/205-tizenbrew`, `agente/205-glassmock`, `agente/205-mock21`,
  `agente/205-perfis21`, `agente/205-respostas`). No confirmed user-facing content.
- **Custom subtitle font** — the other half of #501, no branch. Android.
- **Manual subtitle sync** — `agente/205-legajuste`, spec only. Deferred to 2.1.
- **Open PRs with no local branch**: #456/#457/#466/#467/#468/#469/#478/#482 (out-of-scope ones stay in the
  map). Triage, not lost.

### Merged in this window (not pending any more)

Landing in 2.0.7, already in the sections above:

- **A3 loading screen** (`ace1fd53a`), **watch tracking / ghost seasons** (`91da5935e`, `c0dc799a3`),
  **#545 quota follow-up** (`b03502f84`), **deep links** (`a4f220cd7`), **external player** (`281b4af37`,
  `161cdebd2`), **PR #569 Specials** (`054ccef24`), **#555 search/episodes** (`d5ae66d9e`, `438423097`),
  **#551** (`e680776ed`), **#552** (`0d7821a01`), **#558** (`a1888d007`), **#483** (`251632be`),
  **#529** (`f8fbd3bd3`), **row styles / See all** (`4baf783fa`, `d2b4e0b12`, `76c6e8eea`),
  **Vidaa** (`d0bf5ba79`), **collection header** (`fec2dc0ac`), **PR #431 TV/Mobile auto** (`875d7f18f`),
  **USB P2P on every platform** (`0fe8a7731`, `2ed922f03`, `84c1a90b6`), **buffer size** (`35a944d9f`),
  **trailer audio + captions** (`b43655122`, `31dcf5275`), **P2P reasons** (`e21441cd2`, `30b57ccf1`),
  **glass/frost work** (`46e0bd5bd`, `b5007ae85`, `e9c809475`, `0c77460c1`, `d98d72026`),
  **release 2.0.7 bump and notes** (`b72189c5d`, `e275de18a`),
  **#468 Home/Jellyfin views** (`08c98b017`), **previous-episode button removed** (`d51d4e46c`), the
  **opening-screen style setting A3/B3** (`922500ef4`, `9c3ecdf85`), the
  **revert of the cached-art frost** (`f40a93dc3`, undoing `76bc9d0c3`), the
  **cheaper screen-capture frost** (`4c82273d7`), the **polished friends row** (`f21d993b2`), **PR #561**
  (`86d194462`), **glass tabs** (`a096e5eb8`), **Settings at 100%** (`f22761e28`), the **Trakt comments row**
  (`335bd2abc`), the **frosted context menu + Poster menu setting** (`dc040ef2d`), **PR #574**
  (`2c7c85860`), the **Dynamic Gray/Colour background** (`e3fdbe717`), the **Home frost islands**
  (`f087f4b59`) and the **sidebar frost / glass accent pill** (`ab0e9a597`).

### Frost performance (to fill before tagging)

Placeholder carried from `j-release207`: the measured numbers belong here once the frost work is measured on a
device. Points to report: one backdrop capture per frame on Android/.tpk (`0c77460c1`, `d98d72026`, `5536ee8da`)
and one capture per backdrop change everywhere (`e9c809475`).

**Measured (`4c82273d7`, `/Volumes/ExternalSSD/tmp/foscoleve3/medidas.md`):** blur pixels per frame on the
animated/focused C9 Home at 1080p went from **498,960 to 24,289** (native/webOS and Android), at 4K from
**1,995,840 to 56,621**; passes per capture 5 → 3 (4 at 4K); Explore 47,520/59,400 → 11,394/14,243; Settings,
detail and search keep **0 captures** when static and unchanged. Captures per frame and the copied screen
fraction stay the same as the old path (0.35). All twelve C9 1080p reference shots pass with a maximum
8-bit channel difference of **4** against the old path (limit 5), masking only live clock digits. The
every-other-frame throttle was dropped: a changed draw hash always refreshes that frame. Extra cost: one full
RGB backdrop texture per layer — **5.93 MiB at 1080p, 23.73 MiB at 4K** — and a region miss may blur the
saved backdrop on that frame. No TV GPU timing yet; these are Mac-side counters.

### Merged in this window (not pending any more)

Landing in 2.0.7, already in the sections above:

- **A3 loading screen** (`ace1fd53a`), **watch tracking / ghost seasons** (`91da5935e`, `c0dc799a3`),
  **#545 quota follow-up** (`b03502f84`), **deep links** (`a4f220cd7`), **external player** (`281b4af37`,
  `161cdebd2`), **PR #569 Specials** (`054ccef24`), **#555 search/episodes** (`d5ae66d9e`, `438423097`),
  **#551** (`e680776ed`), **#552** (`0d7821a01`), **#558** (`a1888d007`), **#483** (`251632be`),
  **#529** (`f8fbd3bd3`), **row styles / See all** (`4baf783fa`, `d2b4e0b12`, `76c6e8eea`),
  **Vidaa** (`d0bf5ba79`), **collection header** (`fec2dc0ac`), **PR #431 TV/Mobile auto** (`875d7f18f`),
  **USB P2P on every platform** (`0fe8a7731`, `2ed922f03`, `84c1a90b6`), **buffer size** (`35a944d9f`),
  **trailer audio + captions** (`b43655122`, `31dcf5275`), **P2P reasons** (`e21441cd2`, `30b57ccf1`),
  **glass/frost work** (`46e0bd5bd`, `b5007ae85`, `e9c809475`, `0c77460c1`, `d98d72026`),
  **release 2.0.7 bump and notes** (`b72189c5d`, `e275de18a`),
  **#468 Home/Jellyfin views** (`08c98b017`), **previous-episode button removed** (`d51d4e46c`), the
  **opening-screen style setting A3/B3** (`922500ef4`, `9c3ecdf85`), the
  **revert of the cached-art frost** (`f40a93dc3`, undoing `76bc9d0c3`), the
  **cheaper screen-capture frost** (`4c82273d7`), the **polished friends row** (`f21d993b2`), **PR #561**
  (`86d194462`), **glass tabs** (`a096e5eb8`), **Settings at 100%** (`f22761e28`), the **Trakt comments row**
  (`335bd2abc`), the **frosted context menu + Poster menu setting** (`dc040ef2d`), **PR #574**
  (`2c7c85860`), the **Dynamic Gray/Colour background** (`e3fdbe717`), the **Home frost islands**
  (`f087f4b59`) and the **sidebar frost / glass accent pill** (`ab0e9a597`).

### Frost performance (to fill before tagging)

Placeholder carried from `j-release207`: the measured numbers belong here once the frost work is measured on a
device. Points to report: one backdrop capture per frame on Android/.tpk (`0c77460c1`, `d98d72026`,
`5536ee8da`) and one capture per backdrop change everywhere (`e9c809475`).

**Measured (`4c82273d7`, `/Volumes/ExternalSSD/tmp/foscoleve3/medidas.md`):** blur pixels per frame on the
animated/focused C9 Home at 1080p went from **498,960 to 24,289** (native/webOS and Android), at 4K from
**1,995,840 to 56,621**; passes per capture 5 → 3 (4 at 4K); Explore 47,520/59,400 → 11,394/14,243; Settings,
detail and search keep **0 captures** when static and unchanged. Captures per frame and the copied screen
fraction stay the same as the old path (0.35). All twelve C9 1080p reference shots pass with a maximum
8-bit channel difference of **4** against the old path (limit 5), masking only live clock digits. The
every-other-frame throttle was dropped: a changed draw hash always refreshes that frame. Extra cost: one full
RGB backdrop texture per layer — **5.93 MiB at 1080p, 23.73 MiB at 4K** — and a region miss may blur the
saved backdrop on that frame. No TV GPU timing yet; these are Mac-side counters.
