# #382 — Network health while opening a title

Evidence: local plain-text log `log-382-RSM044.txt`, Nuvio 2.0.5 on webOS,
2026-10-13 11:02. No GitHub or device access was used.

## What the log shows

At lines 843–852, the user opens `movie/tt29355505` (Toy Story 5), at app
clock 199403 ms. Metadata, streams, subtitles and extras start together.
This is a movie detail, despite the comment calling it a show.

| Request / host | Result in the log |
| --- | --- |
| Metadata, `aiometadata.stremio.ru` | Final curl 28, 0 bytes, 5073 ms; connect timeout after 1918 ms (874–875). |
| Streams, `aiostreams-nightly.stremio.ru`, both AIOStreams addons | Final curl 28, 0 bytes; first round 7752/7753 ms; addon retry finishes at 15459/15460 ms with no response (883–887, 915–920). Declared addon deadline is 30 s, but connection timeouts end it earlier. |
| Trailer, `tv.apple.com` | Final curl 28, 0 bytes, 8503 ms; DNS resolution times out after 3499 ms (876–877). |
| Providers, `api.themoviedb.org` | Final curl 28, 0 bytes, 8503 ms; DNS resolution times out after 3500 ms (889–890). |
| Trailer, `api.graphql.imdb.com` | Final curl 28, 0 bytes, 10004 ms; DNS resolution times out after 5000 ms (891–893). |
| Subtitles, `submaker.elfhosted.com` | Final curl 28, HTTP 0, 0 bytes, 8874 ms; connect timeout after 4426 ms (894–897). |
| Quotes, `query.wikidata.org` | Final curl 28, 0 bytes, 10011 ms; DNS resolution times out after 5001 ms (906–907). |
| Ratings, POST to `api.mdblist.com` | Repeated curl 28 connect timeouts, approximately 3850–3900 ms (866, 882, 903, 914, 928, 936, 947). |
| Fallback metadata, `catalog.nuvio.tv` | Final curl 28, HTTP 0, 0 bytes, 3583 ms; connect timeout after 1601 ms (924–927). |
| Fallback metadata, `v3-cinemeta.strem.io` | Final curl 28, 0 bytes, 6263 ms; connect timeout after 2404 ms (935–938). |

These are DNS/TCP failures across distinct hosts, not HTTP 4xx/5xx and not
one slow addon. A later image fetch succeeds in 250 ms at line 979, after
the user leaves the detail. The image log does not identify its host.
`conexao reusada` means CURLINFO_NUM_CONNECTS was zero; a connection attempt
that never completed also has zero new connections. That label alone does
not prove a stale reused socket.

## Decision and correction

The i18n key is `Sem internet. Mostrando o que já estava salvo.` in
`src/ilhasinais.c`, function `rede()`. `ilhasinais_passo()` calls it each
frame. `ilhasinais_iniciar()` registers `rede_saude_nota()` as the network
listener. `src/redesaude.c:rede_saude_passo()` changes state after three
consecutive transport failures, at least two hosts and at least three
seconds, with no success between them. An external success clears offline;
local hosts and cancellation are ignored. The same state also controls the
central clock and log-upload availability.

The defect was in `src/rede.c:rede_baixar_interno3()`: releasing the first
failed attempt through `soltarHandleR()` notified network health *before*
the retry finished. Recovered requests could therefore create an offline
transition and a subsequent recovery transition. `soltarHandleR()` also
ignored a received HTTP status when curl later failed on the response body.

The correction discards an intermediate handle without notifying health
or final-request statistics. Only the completed request decides health.
A received HTTP status counts as connectivity even if the body fails;
the original curl error remains available to the caller and host statistics.
Cancellation and deliberate byte limits remain ignored. Existing retries,
connection reuse, specific addon errors and multi-host offline thresholds
remain in place.

RSM044 does not log health transitions or the displayed message, so the
exact request that triggered the UI cannot be identified from this log.
It does prove that intermediate failures were eligible to trigger it, and
that final failures across many hosts would still justify offline under
the existing rule. The change does not claim to fix the underlying TV DNS
or TCP outage, or prove that this whole outage was a false offline result.

## Validation and remaining device work

`tests/rede_saude.sh` exercises real network wrappers and the health state
with fake curl and a fake clock, with no network access:

- Before the fix, both retry recovery and HTTP-response/body-timeout cases
  fail with rc 134 (assertion failures).
- After the fix, both pass with rc 0. Final failures across distinct hosts
  still set offline, and a failed body after HTTP 503 clears it.
- `tests/redesaude.sh`, `tests/rede_tmdb.sh` (native, Android, TPK40),
  `tests/rede_reuso.sh` and `tests/rede_parada.sh` pass with rc 0. The last
  two use only a loopback HTTP server.

On the affected LG, verify cold boot and opening a title with a recovered
retry, a single unavailable addon, and actual internet loss/restoration.
Capture DNS/TCP behavior during a recurrence to distinguish TV resolver,
router/link and app transport problems. No TV build, push or deployment
was performed.
