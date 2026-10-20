# Nuvio Legacy 2.0.8

Changes integrated after `v2.0.7`. Updated from local commit and branch snapshots; device validation remains pending.
Community: **https://discord.gg/9NWr6SHyzJ**

Source for the in-app What's New card and release notes. English follows the owner's current language override.

## Highlights

None recorded yet.

## New

- **Android cache controls**: Clear regenerable cache while preserving durable user data, APK staging and viable torrent streaming windows; superseded update APKs are removed. Platforms: Android. Refs owner request; d8699b55c, e731a7847, d35f60069, 70b1f55e3; merges a1169f5ad, fcc581387.

## Fixes

- **Posters follow metadata language changes**: Refresh cached catalog posters when the metadata language changes so browsing artwork follows the new selection. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request, `21dc9f41f`, merge `564b557e2`.
- **Clearer frame-rate overlay**: Label the performance overlay as UI fps so its interface frame rate is distinguishable from video playback cadence. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request, `15af443d8`, merge `84eae41cf`.
- **Custom posters across catalogs**: Apply configured poster services consistently across browsing surfaces and support shape-dependent URL templates. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request, `a25def053`, merge `c450d139d`; authenticated service rendering still needs device validation.
- **Expanded-card trailers fill their frame**: Keep focused-card trailer placement in its own coordinate space so video fills expanded artwork at different text scales. Platforms: webOS, Android, Samsung .tpk. Refs owner request, `8271a8dbb`, merge `a4d52842e`.

- **Interface follows your language again**: Restore sorted translation keys so selecting another language no longer leaves the interface in Portuguese. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner incident; `98ed41d91`.
- **Resume survives TV power loss**: Save durable playback checkpoints while keeping tracker playback active during checkpoint updates. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request; 5623ab442, ceca13b7e; merges 9e6c12d47, eb5203637.
- **LG buffer presets apply to direct video**: Apply the chosen buffer preset to direct HTTP video on demand. Platforms: webOS. Refs owner request; 6c9bc485b; merge 6c8233ded.
- **Live diagnostics on limited accounts**: Run explicitly requested live-TV diagnostics even when the account has limited access. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request; 6a392a97f; merge bf6df9a8e.
- **Ranked posters keep their shape**: Keep numbered and stacked ranking rows in portrait format, including artwork loaded ahead of scrolling. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner request; b19d8ec28; merge 35caf5df8.
- **Simkl anime in Continue Watching**: Preserve absolute episode numbers and fallback IDs, using provider metadata to map long-running anime to the next episode. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner report, `1d2ab0a5b`, `af9250ab5`, merge `5d30131d6`. Cached anime bodies now also accept numeric next-episode values. Refs `5bc056384`, merge `0f18626ad`.
- **Complete Simkl library lists**: Keep library entries across all Simkl statuses so watched and watching lists do not lose titles during synchronization. Platforms: webOS, Android, Samsung .tpk, WASM. Refs owner report, `30efe6903`, merge `3e709ec43`.
- **Stable Samsung live picture size**: Reset geometry when switching channels and settle guide transitions once, preventing crops from the previous channel and visible zoom steps. Platforms: Samsung .tpk. Refs owner report, `c3ac7d1ae`, merge `e3cd59e71`.
- **Samsung playback overlays stay visible**: Keep API11 window rendering active over static video holes so playback controls remain drawn. Platforms: Samsung .tpk. Refs owner report, `b3c4a5e36`, merge `f3f5af5ff`. Device smoothness remains unverified.
- **Live playback avoids competing probes**: Skip unnecessary media probes while live channels play, reducing interference with channel playback. Platforms: Samsung .tpk. Refs tester report, `8847255a3`, merge `b206981ce`. Continuous HLS playback still needs device confirmation.
- **Torrent sources start without waiting for optional cues**: Limit speculative tail reads to one second during local torrent startup and continue without optional Matroska cues when those pieces are unavailable. Platforms: webOS, Android, Samsung .tpk; native DTS/P2P path. Refs tester report, `d4cb60161`, merge `0a1a32a13`.
- **Samsung opens at full size**: Ignore the temporary 1×1 startup surface to prevent the black screen that previously required reopening the app. Platforms: Samsung .tpk (Tizen 9/10). Refs #624, #628, #629, `f840b4037`. TV validation pending.
- **No false interrupted-launch notice**: Closing the app normally through the TV no longer triggers a “Previous launch stopped” notice on the next launch. Platforms: Samsung .tpk. Background eviction after normal playback is also treated as a normal close. Refs `36a2d1f13`, `6755ff560`, merge `ad50fc8f5` (no fix issue supplied).
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

- **Translation regression coverage**: Restore language-test coverage and verify translation-key ordering to catch language-selection regressions. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `3b3906186`, merge `d4f8c2845`.
- **C9 artwork comparison**: Verify native C9 artwork against the earlier rendering baseline while preserving both capture modes. Platforms: webOS; host verification. Refs `bdc7d7c8c`, merge `d072c891b`.

- **Browser build link fix**: Restore the browser build's missing network-function stub so Samsung browser packages can link successfully. Platforms: Samsung browser/WASM. Refs `b84091f4c`.
- **Issue tracking**: Keep release and issue tracking current. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `609ed3cc3`, `ad6ec6fb9`, `89b48afe7`, `12bedced8`, `d2a392e14`, `dacee39da`, `9460c0dd8`, `e901b95e7`, `f1743efe5`, `009bba7ae`.

## Pending (not integrated yet)

- **Add-on subtitle synchronization**: Subtitle add-on synchronization work awaits review before its final user-facing behavior can be confirmed. Platforms: Confirm from review. Refs agente/205-205-legaddonsync.
Earlier report named `agente/205-i429hbs` (#429), `agente/205-p2pusb` (#334, #505), `agente/205-i513autoplay` (#513), `agente/205-barra27`, PRs #489/#490/#491/#493/#498, `agente/205-dicatoque` (#472), m3umisto, palco2, logos, fileiraui, diretorpol and triagem-velhas as pending. Their branches are absent from the current open snapshot; retain this historical record without presenting them as pending 2.0.8 features or duplicating already released work.

- **Plugin-only source errors**: Recognize enabled local scrapers when explaining unavailable sources, so plugin-only users do not receive a false missing-add-on message. Platforms: webOS, Android, Samsung .tpk, WASM with local plugins. Refs owner report, `agente/205-205-semaddon`. Finished job has no mapped review log; review and integration pending.
- **Torrent engine follow-up**: Further torrent-engine work remains outside integration and needs review of the newer fix log before its user-visible scope can be confirmed. Platforms: confirm from review before release. Refs `agente/p2p`; current `open.txt` and review snapshot.

Other open branches are historical work or deferred plans; see the tracking map and external `estado.md`. No new feature or external contributor credit is inferred from branch names.

- **Distinct AIOStreams instances**: Keep instances distinct by transport URL; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-aiomulti`.

- **Artwork loading retries**: Keep visible artwork from repeatedly disappearing and reloading; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-imgloop`.

- **Live TV automatic quality**: Correct automatic HLS quality selection and unreliable speed advice; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-livetv720`.

- **Player frame-rate handling**: Keep visible player menus and overlays rendering smoothly; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-playerfps`.

- **Plugin source labels**: Preserve source titles alongside descriptions; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-plugmeta`.

- **Top ranking controls**: Restore missing configured TOP X catalogs; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-topx`.

- **Android tunneled playback**: Offer an optional Android hardware audio/video synchronization setting, disabled by default; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-tunnel`.

- **Dolby Vision playback follow-up**: Update Dolby Vision playback handling; final behavior awaits review and integration. Platforms: confirm from review before release. Refs owner request, `agente/205-205-dv7matis`.
