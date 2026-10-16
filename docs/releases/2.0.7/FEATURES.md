# Nuvio Legacy 2.0.7

Everything merged into `integracao/2.0.5` after the `v2.0.6` tag (base `ae9c3c138`), plus the owner requests
that landed in the same window. Community server: **https://discord.gg/9NWr6SHyzJ**

---

## PT-BR

### Destaques

- **Player externo no Android TV** (#501): escolha qual app de vídeo instalado abre a fonte em
  *Ajustes › Player* (Interno, Externo ou Perguntar sempre) — e uma escolha separada para anime. Legenda e
  proporção sobrevivem. Android. Refs #501, `f954a272c`, `276207fac`, `059f4d4f3`.
- **Cache do P2P no pendrive, em todas as plataformas**: o cache USB passa a aparecer e funcionar no Android e
  no Samsung .tpk — antes só no webOS — com o tamanho e o motivo de cada recusa explicados em português.
  Android, Samsung .tpk. Refs pedidos do dono, `31a62b273`, `51daddd84`, `b090905dc`, `a633888a3`, `ebb5eb8fe`.
- **Modo TV e Celular automático no APK Android** (PR #431): o app escolhe a interface pelo tamanho da tela e
  pelo controle, sem alternar nada na mão. Android. Refs PR #431, `c305e6f7e`, `8979ef16c`, `875d7f18f`.
- **Busca e episódios voltam a funcionar** (#555): séries de título exato como "Bigg Boss" não ficam mais fora
  do limite de resultados, e séries que vêm de um add-on sem episódios preenchem a lista pelo catálogo.
  Todas as plataformas. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catálogos: fileiras fantasmas e fontes que somem** (#545 #551 #552): catálogos de add-on desinstalado saem
  da Home, fileiras que o add-on não declara nesta sessão não ocupam vaga, e as fontes voltam a aparecer
  quando a sonda do add-on demora. Todas as plataformas. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`,
  `455135df2`, `cbcf467f5`, `27988615e`.

### Novidades

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

### Correções

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

### Pendente (ainda não integrado)

Trabalho em branch aberta, sem merge — **não** faz parte da 2.0.7 como está:

- **Idiomas de áudio e legenda informados pelo add-on** — @ghbarker, PR #487 (`agente/205-pr487v2`,
  VEREDITO OK; falta o merge). Android, LG webOS.
- **Perfil de visualizações (#548)** — `agente/205-vistos548`, VEREDITO **BLOQUEIA** com fix feito e sem
  review nova. Android, LG webOS.
- **Deep links** — `agente/205-deeplink`, VEREDITO **BLOQUEIA** com fix feito e sem review nova.
  Todas as plataformas.
- **Resolução de imagem** — `agente/205-imgres`, VEREDITO **BLOQUEIA** com fix feito e sem review nova.
  Todas as plataformas.
- **Contorno da legenda** — `agente/205-legcontorno`, VEREDITO **BLOQUEIA** com fix feito e sem review nova.
  LG webOS, Samsung .tpk.
- **URL de pôster** — `agente/205-posterurl`, VEREDITO **BLOQUEIA** com fix feito e sem review nova.
  Todas as plataformas.
- **Novidades 2.0.7 no app** — `agente/205-novidades207`, VEREDITO **BLOQUEIA**, fix rodando agora.
  Todas as plataformas.
- **Linha de cabeçalho colorida** — `agente/205-colhdr`, VEREDITO **BLOQUEIA**, sem fix.
  Todas as plataformas.
- **PR #493 v2 / v3** (perfil e sessão) — `agente/205-pr493`, 12 commits fora, VEREDITO OK no v3 mas a branch
  seguiu crescendo com BLOQUEIA depois. Android, LG webOS.
- **Fonte de legenda customizada** — a segunda metade de #501, sem branch. Android.
- **Sync manual de legenda** — `agente/205-legajuste`, só especificação. Adiado para 2.1.
- **Tizen/homebrew, glassmock, mock21, perfis21, respostas ao dono** — branches com job feito e nenhum review.
  Sem conteúdo de usuário confirmado.

---

## EN (same list, English copy)

### Highlights

- **External player on Android TV** (#501): pick which installed video app opens the source in
  *Settings › Player* (Internal, External, Ask every time) — plus a separate choice for anime. Subtitle and
  aspect preferences survive. Android. Refs #501, `f954a272c`, `276207fac`, `059f4d4f3`.
- **P2P cache on a USB drive, on every platform**: USB caching now shows up and works on Android and Samsung
  .tpk, not just webOS, with the size limit and the reason for each rejection spelled out.
  Android, Samsung .tpk. Refs owner requests, `31a62b273`, `51daddd84`, `b090905dc`, `a633888a3`, `ebb5eb8fe`.
- **Automatic TV and Mobile mode in the standard Android APK** (PR #431): the app picks the interface from
  screen size and input, nothing to switch by hand. Android. Refs PR #431, `c305e6f7e`, `8979ef16c`, `875d7f18f`.
- **Search and episodes work again** (#555): exact-title series like "Bigg Boss" are no longer cut by the
  result cap, and series that come from an add-on with no episodes get their list filled from the catalog.
  All platforms. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catalogs: ghost rows and vanishing sources** (#545 #551 #552): rows of uninstalled add-ons leave Home, rows
  the add-on does not declare this session no longer take a slot, and sources come back when the add-on probe
  is slow. All platforms. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`, `455135df2`, `cbcf467f5`, `27988615e`.

### New

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

### Fixes

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

### Pending (not integrated yet)

Work on open branches with no merge — **not** part of 2.0.7 as it stands:

- **Add-on reported audio and subtitle languages** — @ghbarker, PR #487 (`agente/205-pr487v2`, VEREDITO OK;
  merge missing). Android, LG webOS.
- **Watch profile (#548)** — `agente/205-vistos548`, VEREDITO **BLOQUEIA** with a fix done and no new review.
  Android, LG webOS.
- **Deep links** — `agente/205-deeplink`, VEREDITO **BLOQUEIA** with a fix done and no new review.
  All platforms.
- **Image resolution** — `agente/205-imgres`, VEREDITO **BLOQUEIA** with a fix done and no new review.
  All platforms.
- **Subtitle outline** — `agente/205-legcontorno`, VEREDITO **BLOQUEIA** with a fix done and no new review.
  LG webOS, Samsung .tpk.
- **Poster URL** — `agente/205-posterurl`, VEREDITO **BLOQUEIA** with a fix done and no new review.
  All platforms.
- **2.0.7 in-app What's New** — `agente/205-novidades207`, VEREDITO **BLOQUEIA**, fix running now.
  All platforms.
- **Coloured header row** — `agente/205-colhdr`, VEREDITO **BLOQUEIA**, no fix. All platforms.
- **PR #493 v2 / v3** (profile and session) — `agente/205-pr493`, 12 commits out, VEREDITO OK on v3 but the
  branch kept growing with BLOQUEIA after. Android, LG webOS.
- **Custom subtitle font** — the other half of #501, no branch. Android.
- **Manual subtitle sync** — `agente/205-legajuste`, spec only. Deferred to 2.1.
- **Tizen/homebrew, glassmock, mock21, perfis21, owner answers** — branches with a finished job and no review
  at all. No confirmed user-facing content.
