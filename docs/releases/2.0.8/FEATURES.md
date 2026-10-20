# Nuvio Legacy 2.0.8

Changes integrated after `v2.0.7`. Updated from local commit and branch snapshots; device validation remains pending.
Community: **https://discord.gg/9NWr6SHyzJ**

Source for the in-app What's New card and release notes. English follows the owner's current language override.

## Highlights

None recorded yet.

## New

None recorded yet.

## Fixes

- **Samsung opens at full size**: Ignore the temporary 1×1 startup surface to prevent the black screen that previously required reopening the app. Platforms: Samsung .tpk (Tizen 9/10). Refs #624, #628, #629, `f840b4037`. TV validation pending.
- **No false interrupted-launch notice**: Closing the app normally through the TV no longer triggers a “Previous launch stopped” notice on the next launch. Platforms: Samsung .tpk. Refs `36a2d1f13` (no issue supplied).
- **Torrent playback with less free memory**: Allow a 160 MB streaming window so TVs with roughly 460 MB free memory can start torrents previously rejected for insufficient space. Platforms: webOS. Refs `8e67f7be0` (no issue supplied).

## Performance

None recorded separately; low-memory playback improvement listed under Fixes.

## Community

No external contribution recorded in the supplied commits since `v2.0.7`.

## Internal

- **Browser build link fix**: Restore the browser build's missing network-function stub so Samsung browser packages can link successfully. Platforms: Samsung browser/WASM. Refs `b84091f4c`.
- **Issue tracking**: Keep release and issue tracking current. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `d2a392e14`, `dacee39da`, `9460c0dd8`.

## Pending (not integrated yet)

Earlier report named `agente/205-i429hbs` (#429), `agente/205-p2pusb` (#334, #505), `agente/205-i513autoplay` (#513), `agente/205-barra27`, PRs #489/#490/#491/#493/#498, `agente/205-dicatoque` (#472), m3umisto, palco2, logos, fileiraui, diretorpol and triagem-velhas as pending. Their branches are absent from the current open snapshot; retain this historical record without presenting them as pending 2.0.8 features or duplicating already released work.

- **Torrent engine follow-up**: Further torrent-engine work remains outside integration and needs review of the newer fix log before its user-visible scope can be confirmed. Platforms: confirm from review before release. Refs `agente/p2p`; current `open.txt` and review snapshot.

Other open branches are historical work or deferred plans; see the tracking map and external `estado.md`. No new feature or external contributor credit is inferred from branch names.
