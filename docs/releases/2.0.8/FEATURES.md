# Nuvio Legacy 2.0.8

Changes integrated after `v2.0.7`. Updated from local commit and branch snapshots; device validation remains pending.
Community: **https://discord.gg/9NWr6SHyzJ**

Source for the in-app What's New card and release notes. English follows the owner's current language override.

## Highlights

None recorded yet.

## New

None recorded yet.

## Fixes

- **Simkl anime in Continue Watching**: Preserve absolute episode numbers and fallback IDs, using provider metadata to map long-running anime to the next episode. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner report, `1d2ab0a5b`, `af9250ab5`, merge `5d30131d6`. Latest review still reports BLOQUEIA; follow-up required before release.
- **Complete Simkl library lists**: Keep library entries across all Simkl statuses so watched and watching lists do not lose titles during synchronization. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner report, `30efe6903`, merge `3e709ec43`.
- **Stable Samsung live picture size**: Reset geometry when switching channels and settle guide transitions once, preventing crops from the previous channel and visible zoom steps. Platforms: Samsung .tpk. Refs owner report, `c3ac7d1ae`, merge `e3cd59e71`.
- **Samsung playback overlays stay visible**: Keep API11 window rendering active over static video holes so playback controls remain drawn. Platforms: Samsung .tpk. Refs owner report, `b3c4a5e36`, merge `f3f5af5ff`. Device smoothness remains unverified.
- **Live playback avoids competing probes**: Skip unnecessary media probes while live channels play, reducing interference with channel playback. Platforms: Samsung .tpk. Refs tester report, `8847255a3`, merge `b206981ce`. Continuous HLS playback still needs device confirmation.
- **Torrent sources start without waiting for optional cues**: Limit speculative tail reads to one second during local torrent startup and continue without optional Matroska cues when those pieces are unavailable. Platforms: webOS, Android, Samsung .tpk; native DTS/P2P path. Refs tester report, `d4cb60161`, merge `0a1a32a13`.
- **Samsung opens at full size**: Ignore the temporary 1×1 startup surface to prevent the black screen that previously required reopening the app. Platforms: Samsung .tpk (Tizen 9/10). Refs #624, #628, #629, `f840b4037`. TV validation pending.
- **No false interrupted-launch notice**: Closing the app normally through the TV no longer triggers a “Previous launch stopped” notice on the next launch. Platforms: Samsung .tpk. Refs `36a2d1f13` (no issue supplied).
- **Torrent playback with less free memory**: Allow a 160 MB streaming window so TVs with roughly 460 MB free memory can start torrents previously rejected for insufficient space. Platforms: webOS. Refs `8e67f7be0` (no issue supplied).

- **Live channels start on LG TVs**: Let playback discover its default tracks before selecting video, preventing premature track selection from blocking TS and HLS startup. Platforms: webOS. Refs #625, `abe573043`, merge `dda4428e4`.
- **Responsive Samsung episode endings**: Keep playback controls responsive during stalls and record episode completion at the end without restarting recovery after playback finishes. Platforms: Samsung .tpk. Refs owner request, `06a178e07`, merge `b22929845`.
- **Home loads with slow DNS**: Bound DNS retries and reuse successful answers across requests so slow resolution does not repeatedly exhaust connection deadlines. Platforms: Samsung .tpk. Refs #630, `25eac3a93`, merge `beb30e5b9`.
- **Older LG TVs stay open after profile selection**: Use text surfaces compatible with SDL 2.0.4 to prevent the missing-symbol crash after choosing a profile. Platforms: webOS (older SDL runtime). Refs owner request, `7dc80ce7c`, merge `921210932`.

## Performance

- **Useful scrapers run first**: Remember recent scraper hits per profile, defer anime-only scrapers for other titles, and show checked, queued and cut counts when no sources are found. Platforms: webOS, Android, Samsung .tpk, WASM with local plugins. Refs owner request, `cfe0215b4`, merge `f215e7fd8`.

Low-memory playback and bounded torrent-startup reads are listed under Fixes.

## Community

No external contribution recorded in the supplied commits since `v2.0.7`.

## Internal

- **Browser build link fix**: Restore the browser build's missing network-function stub so Samsung browser packages can link successfully. Platforms: Samsung browser/WASM. Refs `b84091f4c`.
- **Issue tracking**: Keep release and issue tracking current. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `d2a392e14`, `dacee39da`, `9460c0dd8`, `e901b95e7`, `f1743efe5`, `009bba7ae`.

## Pending (not integrated yet)

Earlier report named `agente/205-i429hbs` (#429), `agente/205-p2pusb` (#334, #505), `agente/205-i513autoplay` (#513), `agente/205-barra27`, PRs #489/#490/#491/#493/#498, `agente/205-dicatoque` (#472), m3umisto, palco2, logos, fileiraui, diretorpol and triagem-velhas as pending. Their branches are absent from the current open snapshot; retain this historical record without presenting them as pending 2.0.8 features or duplicating already released work.

- **Plugin-only source errors**: Recognize enabled local scrapers when explaining unavailable sources, so plugin-only users do not receive a false missing-add-on message. Platforms: webOS, Android, Samsung .tpk, WASM with local plugins. Refs owner report, `agente/205-205-semaddon`. Finished job has no mapped review log; review and integration pending.
- **Android storage follow-up**: Bound regenerable storage and expose cache clearing without breaking APK cache routing or torrent streaming windows. Platforms: Android. Refs owner request, `agente/205-205-armazenamento`, `d8699b55c`. Latest review BLOQUEIA; fix job running, review required afterward.
- **Torrent engine follow-up**: Further torrent-engine work remains outside integration and needs review of the newer fix log before its user-visible scope can be confirmed. Platforms: confirm from review before release. Refs `agente/p2p`; current `open.txt` and review snapshot.

Other open branches are historical work or deferred plans; see the tracking map and external `estado.md`. No new feature or external contributor credit is inferred from branch names.
