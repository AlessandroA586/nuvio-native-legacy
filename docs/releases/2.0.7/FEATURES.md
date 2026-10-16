# Nuvio Legacy 2.0.7

Everything merged into `integracao/2.0.5` after the `v2.0.6` tag (base `ae9c3c138`), plus the owner requests
that landed in the same window. Last tracking sync: 2026-10-16 22:55, HEAD `f0f4f4928`. Community server:
**https://discord.gg/9NWr6SHyzJ**

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
- **Busca e episódios voltam a funcionar** (#555): séries de título exato como "Bigg Boss" não ficam mais fora
  do limite de resultados, e séries que vêm de um add-on sem episódios preenchem a lista pelo catálogo.
  Todas as plataformas. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catálogos: fileiras fantasmas e fontes que somem** (#545 #551 #552): catálogos de add-on desinstalado saem
  da Home, fileiras que o add-on não declara nesta sessão não ocupam vaga, e as fontes voltam a aparecer
  quando a sonda do add-on demora. Todas as plataformas. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`,
  `455135df2`, `cbcf467f5`, `27988615e`.

### Novidades

- **Frost no player e nos menus**: os botões, a barra de controle, os painéis de fonte/legenda/áudio, o
  painel de Estatísticas ao vivo e os menus rápido e de guia parental agora usam o mesmo vidro fosco do
  resto da interface, em vez de painel chapado. Todas as plataformas (onde o vídeo dá para ler pelo GL).
  Refs pedidos do dono, `46e0bd5bd`, `3982ef84b`.
- **Menu do topo translúcido com o frost desligado**: com o Vidro ligado e o "Vidro fosco" desligado, a
  barra do topo (e a ilha de avatar/relógio) fica transparente e tingida em vez de opaca. LG webOS.
  `b5007ae85`.
- **Linha "Cache do P2P" no Android**: a configuração de cache em pendrive aparece e funciona no APK
  Android, como já acontecia no webOS. Android. `9b4f37efb`.
- **Ver tudo em grade nas coleções**: as fileiras de destaque abrem em grade de "Ver tudo" com todos
  os itens da coleção. Todas as plataformas. `4baf783fa`.
- **Episódio certo no scrobble**: o progresso enviado para o Trakt usa a identidade exata do episódio.
  Todas as plataformas. `ff1f1aa25`.
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
- **Log enviado com o começo**: o cabeçalho de inicialização (16 KB) volta a entrar no log enviado, que é o
  que serve para diagnosticar. Todas as plataformas. `84c1a90b6`, `3b12a5d2f`.

### Desempenho

- **Home e vidro muito mais leves no Android (TCL) e no .tpk**: uma captura de fundo por quadro em vez de
  4 cópias e 20 passes de desfoque (antes 4.00 capturas / 20 passes, agora 1.15 / 5.75).
  Android, Samsung .tpk. `0c77460c1`, `d98d72026`, `5536ee8da`, `d7ec89975`.
- **Continuar assistindo não trava a Home**: os catálogos abrem enquanto a fileira é montada.
  Todas as plataformas. PR #538, `cee137de7`, `cfc15aeab`, `8c99638f7`.
- **Buffer preso na memória ativa**: a faixa DTS em uso reserva seu 4 MiB antes de medir a fila.
  Todas as plataformas. `62421bfdc`.
- **Vidro fosco: uma captura por mudança de cena**: o desfoque do vidro agora lê uma cópia em meia
  resolução só da camada de fundo, e apenas quando ela muda — 1,15 → 0,35 capturas por quadro com foco/arte
  em movimento, 0,54 → 0,04 na Exploração e 1 → 0 ao rolar fileiras sobre fundo parado. Todas as plataformas.
  `e9c809475`.
- **Detalhe: menos camadas de tela cheia**: furo e véu do cartão de trailer passam a ser um único desenho, e
  um cartão vizinho quase fora da tela deixa de contar como cheio — 4 → 2 camadas, preenchimento 1,81 → 0,99
  no harness, PNGs idênticos. Todas as plataformas. `5ed6ddfaa`.

### Comunidade

- **Modo TV e Celular automático no Android** — @community, PR #431 (`c305e6f7e`, `875d7f18f`).
- **Idiomas de áudio e legenda informados pelo add-on** — @ghbarker, PR #487 (`e4fb8c08`; revisado, ainda não
  integrado — ver *Pendente*).
- **Continuar assistindo sem travar os catálogos** — @ghbarker, PR #538 (`cfc15aeab`, `a748ebf32`).
- **Retratos do elenco do add-on quando o TMDB falha** — @ghbarker, PR #540 (`a1eca619b`, `8cc5d723d`).
- **Tirar linhas de add-on desinstalado** — @ghbarker, PR #543 (`11e6e8535`, `0d44804f0`).
- **Backdrop completo e qualidade da arte respeitada** — @ghbarker, PR #546 (`946ad1819`, `66299e5f0`).
- **Relógio e ilha de retomada com Up** — @ghbarker, PR #547 (`a7d8b1f58`, `960ec872a`).
- **Lembrar a faixa de áudio escolhida** — @Tolu-Walop-E, PR #550 (`0c2a1ba2c`, `2cec223fd`).
- **URL direta toca sem P2P** — @mark-major, PR #559 (`d25a3e513`, `06724b3ed`).

### Interno

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

### Pendente (ainda não integrado)

Trabalho em branch aberta, sem merge — **não** faz parte da 2.0.7 como está:

- **Perfil de visualizações (#548)** — `agente/205-vistos548`, 8 commits fora (tip `b074d9fd5`).
  VEREDITO **BLOQUEIA** `j-rev-vistos548-r7` (19:48): corrida de ordenação entre "marcar série inteira" e o
  override de um episódio; o fix `b074d9fd5` é mais novo que o review. **Needs review.** Android, LG webOS.
- **PR #487 v2** (idiomas de áudio e legenda informados pelo add-on) — `agente/205-pr487v2`, 3 commits fora,
  VEREDITO OK `j-rev-pr487-v2`, sem commit depois. **Needs final re-review.** Android, LG webOS.
- **PR #493 v2 / v3** (perfil e sessão) — `agente/205-pr493`, 12 commits fora. VEREDITO **BLOQUEIA**
  `j-revg-pr493` (mais novo); `j-revpr493-v3` OK foi antes. Precisa de fix novo + review.
  Android, LG webOS.
- **Vidro fosco: deixar como estava** — `agente/205-foscovelho`, 1 commit fora (`ab02d4aaa`); o job
  `j-fix-foscovelho-r2` (rodada 2 dos achados do review) está com log vazio = QUEUED/RUNNING. Dono relata
  que o frost piorou. Todas as plataformas. **Needs review.**
- **Temporadas e episódios fantasmas** — `agente/205-temporadas`, 1 commit fora (`94d16f7de`):
  ignora `videos[]` de add-on que criam uma temporada fake por episódio. VEREDITO **BLOQUEIA**
  `j-rev-temporadas` (19:48). **Needs review.** Todas as plataformas.
- **Catálogo fora da cota** (follow-up de #545) — `agente/205-pfhome545`, 2 commits fora
  (`975638744`, `53b050753`); VEREDITO **BLOQUEIA** `j-revg-pfhome545` (18:47) e o fix `53b050753`
  é mais novo que o review. **Needs review.** Todas as plataformas.
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
- **Search and episodes work again** (#555): exact-title series like "Bigg Boss" are no longer cut by the
  result cap, and series that come from an add-on with no episodes get their list filled from the catalog.
  All platforms. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catalogs: ghost rows and vanishing sources** (#545 #551 #552): rows of uninstalled add-ons leave Home, rows
  the add-on does not declare this session no longer take a slot, and sources come back when the add-on probe
  is slow. All platforms. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`, `455135df2`, `cbcf467f5`, `27988615e`.

### New

- **Frost in the player and its menus**: the buttons, the control bar, the source/subtitle/audio panels,
  the live Statistics panel and the quick menu / parental guide now use the same frosted glass as the rest
  of the interface instead of a flat panel. All platforms (where the video can be read by GL).
  Refs owner requests, `46e0bd5bd`, `3982ef84b`.
- **Translucent top chrome with frost off**: with Glass on and "Vidro fosco" off, the top bar (and the
  avatar/clock island) is see-through tinted glass instead of opaque. LG webOS. `b5007ae85`.
- **"P2P cache" row on Android**: the USB drive cache setting shows up and works in the standard Android
  APK, as it already did on webOS. Android. `9b4f37efb`.
- **See all grid in collections**: showcase rows open into a full "See all" grid of the collection.
  All platforms. `4baf783fa`.
- **Exact episode identity in scrobbles**: progress sent to Trakt uses the exact episode identity.
  All platforms. `ff1f1aa25`.
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

- **High-quality artwork is fetched at the size it needs** (owner requests): at High quality, w342 posters and
  w780 backdrops stored by the catalog are fetched at the size the draw needs, and the 480px small backdrop
  is no longer served for cards up to 640px. All platforms. `ff1cd6dcd`, `44bf4884a`.
- **Long poster URL templates** (owner requests): pasting a long poster URL template on the phone page is no
  longer cut at 1024 characters, and the native keyboards accept the same limit. All platforms. `0950a8eb8`,
  `df7c93aaf`.
- **MKV audio track remembered**: on MKV with a converted track, the hand pick keeps sticking for the next
  episode, even with two identical tracks. All platforms. `3486c616b`.

### Fixes

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

### Performance

- **Much lighter Home and glass on Android (TCL) and .tpk**: one backdrop capture per frame instead of
  4 copies and 20 blur passes (4.00 captures / 20 passes before, 1.15 / 5.75 now). Android, Samsung .tpk.
  `0c77460c1`, `d98d72026`, `5536ee8da`, `d7ec89975`.
- **Continue Watching no longer blocks Home**: catalogs open while the row is built. All platforms.
  PR #538, `cee137de7`, `cfc15aeab`, `8c99638f7`.
- **Buffer clamp accounts for active DTS**: the in-use range reserves its 4 MiB before queue capacity is
  chosen. All platforms. `62421bfdc`.
- **Frosted glass: one capture per backdrop change**: the glass blur now reads a half-res copy of the
  backdrop layer alone, only when that layer changes — 1.15 → 0.35 captures per frame with focus/art moving,
  0.54 → 0.04 on Explore, and 1 → 0 while rows scroll over a static backdrop. All platforms. `e9c809475`.
- **Detail: fewer full-screen layers**: the trailer card's hole and veil are drawn in one pass, and a
  neighbour card almost off screen no longer counts as full — 4 → 2 layers, fill 1.81 → 0.99 on the harness,
  PNGs byte-identical. All platforms. `5ed6ddfaa`.

### Community

- **Automatic TV and Mobile mode on Android** — @community, PR #431 (`c305e6f7e`, `875d7f18f`).
- **Add-on reported audio and subtitle languages** — @ghbarker, PR #487 (`e4fb8c08`; reviewed, not merged —
  see *Pending*).
- **Continue Watching without blocking catalogs** — @ghbarker, PR #538 (`cfc15aeab`, `a748ebf32`).
- **Keep add-on cast portraits when TMDB fails** — @ghbarker, PR #540 (`a1eca619b`, `8cc5d723d`).
- **Remove uninstalled add-on rows** — @ghbarker, PR #543 (`11e6e8535`, `0d44804f0`).
- **Full recommendation backdrop and artwork quality** — @ghbarker, PR #546 (`946ad1819`, `66299e5f0`).
- **Reach the clock and resume island with Up** — @ghbarker, PR #547 (`a7d8b1f58`, `960ec872a`).
- **Remember the hand-picked audio track** — @Tolu-Walop-E, PR #550 (`0c2a1ba2c`, `2cec223fd`).
- **A direct URL plays without P2P** — @mark-major, PR #559 (`d25a3e513`, `06724b3ed`).

### Internal

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

### Pending (not integrated yet)

Work on open branches with no merge — **not** part of 2.0.7 as it stands:

- **Watch profile (#548)** — `agente/205-vistos548`, 8 commits out (tip `b074d9fd5`). VEREDITO
  **BLOQUEIA** `j-rev-vistos548-r7` (19:48): whole-series mark vs single-episode override ordering race;
  the fix `b074d9fd5` is newer than the review. **Needs review.** Android, LG webOS.
- **PR #487 v2** (add-on reported audio and subtitle languages) — `agente/205-pr487v2`, 3 commits out,
  VEREDITO OK `j-rev-pr487-v2`, no commit after. **Needs final re-review.** Android, LG webOS.
- **PR #493 v2 / v3** (profile and session) — `agente/205-pr493`, 12 commits out. VEREDITO **BLOQUEIA**
  `j-revg-pr493` (newer); the OK `j-revpr493-v3` came before. Needs a new fix + review.
  Android, LG webOS.
- **Frosted glass: put it back as it was** — `agente/205-foscovelho`, 1 commit out (`ab02d4aaa`); the
  round-2 fix job `j-fix-foscovelho-r2` has an empty log = QUEUED/RUNNING. Owner reports frosted glass got
  worse. All platforms. **Needs review.**
- **Ghost seasons and episodes** — `agente/205-temporadas`, 1 commit out (`94d16f7de`): ignores add-on
  `videos[]` that create one fake season per episode. VEREDITO **BLOQUEIA** `j-rev-temporadas` (19:48).
  **Needs review.** All platforms.
- **Catalog outside the quota** (#545 follow-up) — `agente/205-pfhome545`, 2 commits out
  (`975638744`, `53b050753`); VEREDITO **BLOQUEIA** `j-revg-pfhome545` (18:47) and the fix `53b050753`
  is newer than the review. **Needs review.** All platforms.
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
