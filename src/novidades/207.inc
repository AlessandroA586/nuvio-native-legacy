// Conteudo da 2.0.7: docs/releases/2.0.7/NOVIDADES.md.
// Sem cenas nem artes. Tetos de linhas para as traducoes; o motor mede antes de paginar.
static const NovItem ITENS_207[] = {
  { .grupo = "Correções", .icone = "aj_puzzle", .nome = "Fontes",
    .frase = "Add-ons como StreamBridge e Emby voltam a mostrar fontes com link direto.", .linhas = 2 },
  { .icone = "aj_search", .nome = "Busca",
    .frase = "O título exato aparece primeiro; episódios achados mesmo sem lista do add-on.", .linhas = 2 },
  { .icone = "aj_droplet", .nome = "Vidro fosco",
    .frase = "Telas mais leves com o vidro fosco ligado.", .linhas = 2 },
  { .grupo = "Novidades", .icone = "aj_play", .nome = "Continuar assistindo",
    .frase = "Até 40 itens na fileira.", .linhas = 2 },
  { .icone = "aj_hard-drive", .nome = "Cache no pendrive",
    .frase = "Agora no Android e na Samsung (.tpk), com detalhes do espaço.", .linhas = 2,
    .plataformas = NOV_ANDROID | NOV_TPK | NOV_OUTRAS },
  { .icone = "aj_memory-stick", .nome = "Buffer do player",
    .frase = "Escolha o tamanho do buffer nos Ajustes.", .linhas = 2 },
  { .icone = "aj_monitor-play", .nome = "Player externo",
    .frase = "Lista os players instalados e permite um player só para anime.", .linhas = 2,
    .plataformas = NOV_ANDROID | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Links externos",
    .frase = "Abra títulos do Nuvio a partir de outros apps.", .linhas = 2,
    .plataformas = NOV_ANDROID | NOV_OUTRAS },
};
static const NovConteudo NOV_ATUAL = {
  .versao = "2.0.7",
  .arquivo = "novidades-207.txt",
  .subtitulo = "Fontes, busca e cache no pendrive.",
  .itens = ITENS_207, .nItens = (int)(sizeof ITENS_207 / sizeof *ITENS_207),
};
