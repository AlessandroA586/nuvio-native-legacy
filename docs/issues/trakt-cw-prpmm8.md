# Trakt: "a seguir" fora do Continuar a Assistir e selo de visto so apos abrir (log PRPMM8)

Relato (Reddit, u/Habibi-1, app 2.0.5 Android): episodios "a seguir" do Trakt nao
aparecem no Continuar a Assistir; cartaz de serie so ganha o selo de visto depois
de abrir o titulo e voltar. Log analisado: `log-PRPMM8.txt` (1891 linhas, so lido,
sem rede; nenhum token/URL de addon copiado).

## Linha do tempo no log

- Conta Trakt conectada: acoes autenticadas funcionam —
  `[trakt] watchlist add tt33311069 (movie) -> confirmado (HTTP 201)` (linha 600) e
  `watchlist del ... confirmado (HTTP 200)` (274, 447).
- O log comeca ~12 s depois do arranque (`hero: foco/abertura` em `[t] 12253`) e
  NAO tem nenhuma linha do sync de arranque do Trakt: nem `[trakt] historico: N
  serie(s)...`, nem `[trakt] a seguir: ...`, nem `[trakt] N em andamento`. Limite
  do log, nao prova de falha; so da para afirmar o que acontece ao abrir titulos.
- Ao abrir o detalhe de uma serie, e so ali que o progresso por serie e lido:
  `[vistoep] tt9288030: consulta trakt progress/watched` (1110), 32 linhas
  `T1E1 ... T4E8: trakt completed=true`, e
  `[visto] marcado por trakt-importacao ... id=tt9288030 n=32 (sem reproducao local)`
  (1163). Idem `tt0981456` (1634). So `progress/watched` aparece 2 vezes no log todo,
  ambas disparadas pela abertura do titulo.
- Nada no log lê o Trakt para series em segundo plano: coerente com os dois sintomas.

## Causas

A. "A seguir" ausente. A lista de candidatas vinha so de `/sync/history?limit=100`
   (ultimas 100 reproducoes, no maximo 64 series em `ult[]`). Serie vista no app do
   Trakt (ou importada) e fora dessas 100 nunca virava candidata, entao
   `consultarProximos` (`/shows/<id>/progress/watched`) nem era chamado para ela.
B. Selo sem abrir. O mapa de "visto" de serie (`cat_visto`) so era escrito em
   `extras.c` ao abrir o detalhe (contadores aired/completed de progress/watched).
   O sync de arranque nunca lia contadores por serie, e `/sync/history` nao marca
   serie inteira (so episodios).

## Correcao (src/trakt.c)

- `carregarSeriesVistas`: `/sync/watched/shows?extended=noseasons` paginado (leve,
  sem mapa de episodios), junto do historico. `unirSeriesVistas` junta ao `ult[]`
  as series que o historico recente nao cobriu (mais novas primeiro, ate o limite),
  com temporada -1 = episodio desconhecido; o "proximo" so vem do
  `/progress/watched` (sem ele, sem palpite).
- `fioProximo`: o mesmo GET de progresso grava o selo (`aired`/`completed` do
  cabecalho => serie vista), sem abrir o detalhe. O GET sai mesmo com o toggle
  "Proximo episodio" desligado; o card continua respeitando os toggles do Trakt no
  Continuar e os filtros de ocultar vistos.
- Teste: `bash tests/trakt_cw_vistas.sh` (rede falsa). Falhava antes, passa depois.
