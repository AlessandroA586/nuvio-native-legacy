# Issue #565 — https://pengu.uk/ on Samsung .wgt (2.0.6)

## Verdict

No fixture-backed app bug remains on current `integracao`. PenguPlay's supplied manifest and unauthenticated stream work through the local install/probe/parser/filter path. The relevant behavioral fix was already made after 2.0.6 by #552.

## What PenguPlay actually returns

- `GET https://pengu.uk/` returns `302 Location: /web/index.html`, then HTML — not a manifest. The app must not fetch the entered root URL as the manifest; it must append `/manifest.json` to normalized base `https://pengu.uk`.
- `manifest.json` declares object-form `resources`: `stream` for `movie`, `series`, `tv`; `meta` and `catalog` only for `tv`; `subtitles` for `movie`/`series`; empty `catalogs`.
- `behaviorHints.configurable=true` means the root install is a configuration page. A configured install URL carries a config token in its path.
- Unauthenticated `/stream/movie/tt...json` returns a playable public URL: `https://pengu.uk/signin.mp4`, with `behaviorHints.notWebReady=false`.

## Current code path

- URL install/normalization: `nv_addon_base` in `src/addonurl.h` trims whitespace, maps `stremio://` to `https://`, removes trailing `/manifest.json` and trailing slashes while preserving config path/query. It is the same shape as official Nuvio `canonicalizeUrl`/`buildManifestUrl`.
- Add-on insertion: `adicionarSemTrava` in `src/addons.c` stores normalized base and starts with optimistic capabilities until probe.
- Manifest probe: `sondar` fetches `<base>/manifest.json` (never the entered `/`), and `capacidadesDoManifesto` accepts resources as strings or objects, case-insensitive names, and type `tv`.
- Stream request: `fioFontes` builds `<base>/stream/<type>/<id>.json`; `nv_addon_id` percent-encodes `tmdb:`/`pp-live:`.
- Stream parser: `src/stream_parse.c` parses the sign-in stream and `notWebReady=false`.
- Offer filter: current `fonteOferecida`/`pedeP2P` in `src/streams.c` no longer reject a public HTTP(S) stream merely because `notWebReady` is true; only non-HTTP or local streaming-server P2P sources require P2P.

## 2.0.6 difference

At `v2.0.6`, `fonteOferecida` was:

```c
return (!s->notWebReady || p2p_ativo()) &&
       (s->url[0] || debrid_ativo() || p2p_ativo());
```

Any public `notWebReady:true` HTTP(S) stream was discarded when P2P was off. Current integration contains #552:

- `6d524f947` — offer `notWebReady` streams that carry an HTTP(S) URL with P2P off.
- `cbcf467f5` — parse stream hostnames before requiring P2P.

Regression `tests/pengu.sh` extracts that exact 2.0.6 predicate into a generated probe and proves it fails the controlled `notWebReady:true` PenguPlay stream; the real current native and `-D__EMSCRIPTEN__` paths pass.

## Fixture coverage added

`tests/pengu.c` + `tests/pengu.sh`, fixtures under `tests/fixtures/pengu/`:

- bare `https://pengu.uk/`, `manifest.json`, configured path, trailing slash, whitespace, and `stremio://` configured URL all normalize correctly;
- manifest probe always requests `<base>/manifest.json`, never `/` or the HTML redirect;
- object resources are read as stream/meta/catalog/subtitles; `tv`, `idPrefixes`, and empty catalogs accepted;
- supplied sign-in stream parses and is offered;
- controlled `notWebReady:true` variant fails under copied 2.0.6 predicate and passes under current code, including `.wgt` macro selection.

No production diff was needed.

## Remaining caveat

The report has no log and no authenticated/configured response. If the user's real PenguPlay stream is an authenticated `notWebReady:true` URL plus required `proxyHeaders`, Tizen may still fail at playback: `src/video_tizen.c` stores `video_definir_cabecalhos` but intentionally does not apply headers to AVPlay. The fixture does not prove that scenario because the supplied stream has no required headers.
