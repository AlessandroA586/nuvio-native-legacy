# Nuvio Legacy 2.0.7

Polish and fixes after 2.0.6, plus the work ported from community PRs. Community server: **https://discord.gg/9NWr6SHyzJ**

## Highlights

- **New cinematic loading screen.** Minimal and calm: sharp artwork, a half-strength veil, and a small ring under the logo.
- **Frost glass everywhere.** The quick menu, the parental guide, the player UI (buttons, control bar, source/subtitle/audio panels) and the live Statistics panel now use the same frosted material as the rest of the app.
- **Transparent glass top menu.** With Glass on and frost off, the top bar and the clock island are see-through tinted glass instead of opaque.
- **Sources sheet grows from the clock island.** The detail sheet now opens out of the island instead of popping in.
- **Season 0 "Specials" shown and playable** (#556, PR #569), and **fake one-episode seasons removed** (for example *The Blacklist*, where an add-on sent one season per episode).
- **Watched state stays in sync** (#548): watched marks survive restarts and stale pulls, and tracker writes no longer race each other.
- **Deep links** (#539): `nuvio-legacy://`, `stremio://` and IMDb links open a title, search, or start playing.
- **External player on Android TV** (#501), **automatic TV/Mobile mode** in the standard APK (PR #431), and **P2P cache on a USB drive** on Android and Samsung .tpk.
- **Frost performance work.** <!-- PLACEHOLDER: frost performance numbers/line to be filled when that work lands -->

## New

- Cinematic loading screen (sharp art, half veil, ring below the logo).
- Frost glass in the player UI, quick menu, parental guide and Statistics panel; transparent glass top chrome with frost off.
- Sources sheet grows from the clock island.
- **See all** grid for Showcase rows in collections (PR #568) and for each catalog's search results (PR #563).
- **Exact episode identity in Trakt scrobbles** (PR #560) and series identity kept through lookup and playback (PR #561).
- Season 0 **Specials** listed and playable (#556, PR #569).
- **Split hero** row style (Settings › Home appearance › Row style), with an animated picker.
- **Trailer audio** setting (Automatic, Original or a language) that prefers the dubbed trailer; **captions** in the YouTube trailer.
- **Buffer size** setting: Automatic, 64, 128 or 256 MiB, clamped to free RAM.
- **Continue Watching up to 40 items** without blocking Home (PR #538).
- **Deep links**: `nuvio-legacy://`, `stremio://`, imdb.com (#539).
- **External player** choice on Android TV, with a separate one for anime (#501).
- **Automatic TV and Mobile mode** in the standard Android APK (PR #431); mobile Settings can reorder add-on blocks and show inline save feedback.
- **P2P cache (USB)** row and full drive details on Android; Tizen USB roots enumerated on Samsung .tpk.
- **Simkl anime** joins the series lists, Continue Watching and Plan to Watch.
- Reach the clock and resume island with **Up** (PR #547).
- YouTube trailer **captions** follow the subtitle language.

## Improved

- Frosted glass is much cheaper: one backdrop capture per frame on Android and .tpk, and one capture per backdrop change everywhere.
- Detail page uses fewer full-screen layers over the live trailer card.
- Continue Watching no longer holds up the Home catalogs.
- Buffer clamp reserves the active DTS cache.
- High image quality fetches posters and backdrops at the size the draw needs; recommendation backdrops are upgraded (PR #546).
- Add-on cast portraits stay when TMDB fails; accented and emoji names no longer duplicate (PR #540).
- Detail artwork is reused while the video opens (PR #541).
- Settings explain why P2P is off and why P2P cache and Space limit are greyed out.
- Subtitle outline is a solid opaque 8-direction ring; the chosen opacity applies once.
- Uploaded logs keep the startup header.
- Long poster URL templates are no longer cut at 1024 characters.
- English-only diagnostic strings so the translation scan stays clean.

## Fixed

- **Fake one-episode seasons** from add-on `videos[]` are ignored (*The Blacklist*); sparse valid add-on catalogs are preserved.
- **Specials / season 0** kept instead of dropped (#556, PR #569); absent season preserved for Kitsu torrent selection.
- **Watched sync** (#548): intent persisted across restarts and stale pulls, retries revalidated, tracker writes serialized, newer episode overrides replayed after whole-series writes.
- **Search drops exact-title series** behind the result cap (#555); episodes filled from the catalog for `tmdb:` ids with no IMDb.
- **Ghost catalog rows** of uninstalled add-ons removed (#545, #504, PR #543); undeclared catalogs no longer take Home slots; explicit choices survive quota exclusion.
- **Sources vanishing** (#552): hostname parsing before requiring P2P, subtitle capability assumed until the manifest probe answers, a direct http(s) URL plays without P2P (PR #559).
- **PenguPlay add-on** (#565): `notWebReady` http streams are offered with P2P off; tests added.
- **Stale profile session** recovered and PIN-completed choice opens the session (#551, PR #554).
- **Black screen on launch** (#558): Samsung IME setup deferred and failures surfaced.
- **Trailers**: fullscreen and dubbed YouTube failures fall back to Apple/IMDb; trailer language honoured at detail and Home; focused-card trailers decoupled from hero visibility.
- **Audio track remembered** for the whole show, including MKV converted tracks (PR #550).
- **Second Try tap** no longer starts internal playback while the external-player ask modal is open.
- **USB with an unknown filesystem** fails closed; USB cache rejections are diagnosed.
- Search opens the selected title; voice startup failures reported.
- Row style picker: closing animates, hit targets disabled while closing, drawn above the menu veil.
- Detail: focus tooltip dropped from the shuffle button; split-row hero copy restored.
- Deep links: pending state reset on replacement, TMDB title matched by media type, season zero rejected for links.
- Android: preload buffer sizes preserved in MiB; first-run log tip stays in the Mobile canvas.

## Community

Thank you to everyone who sent code, reported bugs and tested builds.

**Pull request authors**

- @ghbarker — #538 Continue Watching, #540 cast portraits, #541 title-page artwork, #543 add-on rows, #546 artwork, #547 clock with Up, #560 Trakt scrobbles, #561 series identity, #563 search See all, #568 Showcase See all, #569 Specials, #571 external playback, #431 TV/Mobile mode.
- @Tolu-Walop-E — #550 audio track memory, #554 PIN profile catalogs, #572 remux start.
- @mark-major — #559 direct URL without P2P.
- @Namer03 — #567 source fixes.

**Issue reporters** (open and closed in this window)

- #545 Catalog Now Showing — @whoareux3
- #548 Mark as watched not working — @Hov1122
- #551 Catalog Not Loading 2.0.6 — @somprakashpathakoneplus-cell
- #552 The add-on sources aren't appearing — @mackojanko
- #555 Search does not work as expected — @somprakashpathakoneplus-cell
- #556 Special / out-of-order seasons recognised by Trakt are not shown for some anime — @redfieldchristabel
- #558 Black screen on launch — @JoeBloggs8889
- #565 PenguPlay add on doesn't work — @wanderingikigai95
- #501 [Android TV] Add External Player Selection and Custom Subtitle Font Support — @MrPhenix007
- #539 Feature Request: Add Deep Link Support to Open Specific Movies and TV Shows — @mohammadmrh12
- #505 My P2p add on sources don’t load with version2.05 — @bacs2ponce-ai
- #504 Catalog rows from removed add-ons remain in the “Not on home” section — @rawldon
- #502 Detail page keeps the low-res thumbnail instead of loading full artwork when opened from Recommendations or Filmography — @Yatin-Code
- #528 [Bug/FR] Cast Images Fail to Load — Request for Metadata Add-on Fallback or Custom TMDB Proxy — @LeoAj2005
- #429 Subtitles auto select — @gecXMAN
- #434 Ability to remove genre tags in title page — @Owlphibia29
- #407 Top menu open when scrolling up on the title page — @mackojanko
- #401 Long update time on Homebrew channel (15 mins +) — @Vidhin05

## Pull requests

Ported PRs (the originals may stay open on GitHub; their commits are integrated here):

- #538 Home: prioritize Continue Watching without blocking catalogs — @ghbarker
- #540 Metadata: keep addon cast portraits when TMDB is unavailable — @ghbarker
- #541 Player: reuse title-page artwork while opening video — @ghbarker
- #543 Catalogs: remove uninstalled addon rows and collection sources — @ghbarker
- #546 Artwork: complete recommended-title backdrops and honor High quality — @ghbarker
- #547 Navigation: reach the clock and resume island with Up — @ghbarker
- #550 fix(player): Remember the hand-picked audio track for the whole show (exact track within the same binge group) — @Tolu-Walop-E
- #554 fix(profiles): load Home addon rows after unlocking a profile with its PIN — @Tolu-Walop-E
- #559 fix(sources): a direct URL plays without P2P; notWebReady cannot hide it — @mark-major
- #560 Trakt: send exact episode identities when scrobbling — @ghbarker
- #561 Metadata: preserve series identity during lookup and playback — @ghbarker
- #563 Search: add See all for each catalog’s results — @ghbarker
- #567 Source fixes + view all files + seek problem fix... — @Namer03
- #568 Collections: add See all to Showcase rows — @ghbarker
- #569 Episodes: show and play Specials without dropping season zero — @ghbarker
- #571 Android: add external playback through the system player chooser — @ghbarker
- #572 perf(android): faster start for large remuxes (parallel end-of-file index read, remembered add-on redirect) — @Tolu-Walop-E
- #431 Android: detect TV/Mobile and gate touch paths by the saved mode — @ghbarker

## Issues closed or addressed

- #545 Catalog Now Showing — @whoareux3
- #548 Mark as watched not working — @Hov1122
- #551 Catalog Not Loading 2.0.6 — @somprakashpathakoneplus-cell
- #552 The add-on sources aren't appearing — @mackojanko
- #555 Search does not work as expected — @somprakashpathakoneplus-cell
- #556 Special / out-of-order seasons recognised by Trakt are not shown for some anime — @redfieldchristabel
- #558 Black screen on launch — @JoeBloggs8889
- #565 PenguPlay add on doesn't work — @wanderingikigai95
- #501 [Android TV] Add External Player Selection and Custom Subtitle Font Support — @MrPhenix007
- #539 Feature Request: Add Deep Link Support to Open Specific Movies and TV Shows — @mohammadmrh12
- #505 My P2p add on sources don’t load with version2.05 — @bacs2ponce-ai
- #504 Catalog rows from removed add-ons remain in the “Not on home” section — @rawldon
- #502 Detail page keeps the low-res thumbnail instead of loading full artwork when opened from Recommendations or Filmography — @Yatin-Code
- #528 [Bug/FR] Cast Images Fail to Load — Request for Metadata Add-on Fallback or Custom TMDB Proxy — @LeoAj2005
- #429 Subtitles auto select — @gecXMAN
- #434 Ability to remove genre tags in title page — @Owlphibia29
- #407 Top menu open when scrolling up on the title page — @mackojanko
- #401 Long update time on Homebrew channel (15 mins +) — @Vidhin05

## Commits since v2.0.6

- `4a443422b` fix: preserve absent season for Kitsu torrent selection
- `718e61abb` feat(player): A3 cinematic loading screen (sharp art, half veil, ring below logo)
- `0bfe5ce71` fix(tests): restore isolated discovery harness linking
- `8dff0e2a9` fix(episodes): show and play Specials without dropping season zero
- `fbe06f5e7` fix(sources): grow detail sheet from the clock island
- `288e531ee` fix(episodes): preserve valid sparse addon catalogs
- `620f041d8` docs(map): sync tracking
- `6db247078` fix: replay newer episode overrides after whole-series writes
- `4baf783fa` feat(collections): open Showcase rows in See all grids
- `46e0bd5bd` feat(glass): frost player glass over live video with baked title art
- `61d73a5bf` fix(player): reuse selected detail artwork during opening
- `e0ddeca5f` fix(search): expand catalog windows while retaining exact-title promotion
- `b074d9fd5` fix: skip superseded watched gestures in initial workers
- `3982ef84b` fix(glass): quick menu and agenda panel use the shared frosted material, no opaque base
- `ff1f1aa25` fix(trakt): send exact episode identities in scrobbles
- `94d16f7de` fix(episodes): ignore addon videos[] with one fake season per episode
- `9bc31cc9d` fix: align unmark tests with retained watched intent
- `9b86a6af8` docs(map): sync tracking
- `b5007ae85` fix(glass): keep the top chrome translucent with frost off
- `9b4f37efb` fix(settings): list P2P cache (USB) row on Android too
- `5b37cecd8` fix: emit weak defaults so isolated fixtures link on Mach-O
- `e9c809475` perf(glass): blur only the backdrop layer, once per backdrop change
- `5ed6ddfaa` perf(detail): fewer full-screen layers over the live trailer card
- `1c1dfa50e` feat(novidades): richer 2.0.7 card with highlights, new features and fixes
- `f5dab2f8a` Simkl: merge the third anime type into the series lists
- `c9c2d3f15` fix(detail): drop focus tooltip from shuffle button
- `33b0d7de0` test: link fontefiltro/linguas and stub new helpers after master merge
- `5a926a863` test(home): stub optional addons hooks so home_layout links on Mach-O
- `c76ed4632` test(colordem): stub the optional account addons symbols fileiras.c weak-imports
- `d0f6f5986` Compose subtitle outline once so the selected opacity applies once
- `eb03a56a4` test(colhdr): rename mock runner to *_shot so GL mock stays out of default suite
- `61417ac95` docs(map): sync tracking
- `c32a87bbf` test(streams): #565 PenguPlay auth fixtures — notWebReady http streams offered with P2P off
- `68cf2c63b` test(addons): cover PenguPlay install and stream handling (#565)
- `9356bc16c` test(detail): add NV_FPS frame-cost harness for detail open and episode swipe
- `df7c93aaf` fix: align phone textarea maxlength with poster template limit
- `ac616c0bd` test: link artetamanho.c in artereserva fallback test
- `8330673a8` fix: revalidate watched retries and serialize tracker writes
- `f44e64863` fix: reset pending source state before deep link replacement
- `6a76a46aa` Draw the default subtitle outline as a solid opaque 8-direction ring
- `ff1cd6dcd` fix(art): High image quality upgrades TMDB w342 posters and w780 backdrops to the size the draw needs
- `0950a8eb8` fix: preserve long custom poster URL templates
- `2d04fbecc` fix: isolate watched retries and preserve restored episode marks
- `7cf145dca` fix: preserve pending deep links and reject season zero
- `a779b3dcd` feat(novidades): 2.0.7 what's-new card in 29 languages and release notes
- `44bf4884a` fix(art): High image quality no longer serves the 480px metahub small backdrop for cards up to 640px
- `a6472f80d` fix(home): decouple focused card trailers from hero visibility
- `276207fac` fix: isolate external player package preferences
- `a3eb1e19a` fix: match deep link TMDB titles by media type
- `da2c69701` fix: declare discovery retry and restore sync test handshake
- `059f4d4f3` Pick installed external video player, separate setting for anime
- `0c77460c1` perf(glass): share one frosted backdrop capture per frame on Android too
- `b090905dc` Show Cache do P2P setting on Android
- `8f38bb943` Add deep links: nuvio-legacy://, stremio:// and imdb.com open title/play/search
- `a633888a3` Show full USB drive cache details in the P2P cache modal; OK now closes it
- `f87c41ed7` fix(contapend): guard NULL tipo in contapend_estado series-match
- `d479a8463` Fill episodes from the Nuvio catalog for tmdb: ids with no IMDb in the add-on meta (#555)
- `840dacb67` test: fix Falso initializer for char-array id in cw205 montagem mode
- `5536ee8da` perf(glass): reuse every backdrop capture of a static frame
- `2e5b2ffde` Fix search dropping exact-title series behind the result cap (#555)
- `66a608ef7` fix: persist watched intent across restart and stale pulls (#548)
- `e23f0476d` feat(trailer): YouTube trailer shows captions in the subtitle language
- `31a62b273` P2P: enumerate Tizen USBDrive* roots, add externalstorage privilege
- `8d8f5c92a` tests: link curadoria/ajlog/fonteregra and profile-session stubs for cw tests
- `696a3a6ee` Publish every Continue Watching future up to the row cap
- `4cbf5ad5c` Raise Continue Watching native cap from 12 to 40
- `6d537cb92` fix(ajustes): align foraDestaPlataforma with TELA gates after trailer merge
- `8c99638f7` Tests: cover independent Continue Watching work and catalog startup
- `82dad7d43` docs(map): sync tracking
- `27988615e` fix(addons): assume subtitle capability until the manifest probe says otherwise (#552)
- `a04f2a820` fix(player): hold second tentar while extplayer ask modal is open, fix AJ_N invariant (#501)
- `bd6e61119` Tests: cover addon removal and collection snapshots with current ownership APIs
- `5f4caef1c` fix(trailer): fullscreen Watch trailer falls back like autoplay
- `3486c616b` Preserve MKV track identity for converted audio memory
- `a2fd7e41d` fix: open selected search titles and report voice startup failures
- `171e49772` fix(discovery): English friends-activity log reason so the i18n scan stays clean
- `1797b5306` fix(trailer): dubbed YouTube failure falls back to Apple/IMDb; audio keys caches
- `f954a272c` feat(player): external player setting for Android TV (#501)
- `cc49341e6` fix(tpk): defer Samsung IME setup and expose startup failures
- `d25a3e513` fix(sources): a direct URL plays without P2P, notWebReady cannot hide it
- `c0ea07466` fix(trailer): one Trailer audio rule where YouTube plays; wait for TMDB
- `a06447082` Tests: cover metadata portraits, identity and profile ownership
- `455135df2` Catalogs: remove uninstalled addon rows and owned collection sources
- `a0e225043` Metadata: retain valid cast entries and distinct Unicode identities
- `40b7d8e15` fix: disable style picker hit targets while closing
- `773c576f7` Metadata: keep addon cast portraits when TMDB is unavailable
- `8979ef16c` test(android): cover automatic interface mode and retained gestures
- `7163e053c` Yield retained PiP to Mobile Settings editors
- `acc47411f` fix(android): show inline settings save feedback on mobile
- `29a516106` fix(android): close unavailable settings explanations by touch
- `4bd7a33cc` fix: expose add-on block reordering on mobile
- `510007bdd` Keep the first-run log tip inside the Mobile canvas
- `c305e6f7e` Add automatic TV and Mobile modes to the standard Android APK
- `cee137de7` Prioritize Continue Watching without blocking Home catalogs
- `86c763a2d` fix(ctxmenu): draw closing Row style picker after the menu veil
- `1c7455e37` fix: honor trailer language at native detail and home callers
- `f8815746a` fix(home): restore top hero copy when returning from Split row
- `f8bc42b11` fix(glass): recapture backdrop after intervening modal veil
- `35a944d9f` fix(android): preserve preload buffer sizes in MiB
- `c831a592f` fix: honor original trailer language and prioritize dubbed candidates
- `669fa7b5c` fix(home): year/rating under Split hero posters, hide top hero text while split row focused
- `6375a1520` fix(home): English row-cut diagnostic so the i18n scan stays clean
- `3b025a959` fix(p2p): fail closed on unknown USB filesystems and retain active limit
- `84239e65a` fix(ctxmenu): animate Row style picker closing on Back; fix OOB read in test
- `21d1783b6` fix(fileiras): preserve explicit selection across quota exclusion
- `62421bfdc` fix(player): reserve active DTS cache in buffer memory clamp
- `35efbf90a` fix(p2p): English USB log reason so the i18n scan stays clean
- `d98d72026` perf(glass): share one backdrop capture per frame on .tpk, skip unread draws
- `8655956cb` feat(home): optional "Split hero" row style
- `b43655122` feat(trailer): Trailer audio setting ranks the dubbed trailer first
- `51daddd84` feat(p2p): find USB volumes on Android via getExternalFilesDirs and cap FAT32
- `ba38888fc` docs(map): sync tracking
- `2e480806c` fix(ajustes): explain disabled P2P before USB cache advice
- `67985ea5d` fix(fileiras): preserve quota exclusions during registration
- `92656f2ae` feat(ctxmenu): animate Row style picker opening
- `4907caa50` docs: Dolby Vision profile 7 to 8.1 conversion spec (not implemented)
- `42b3718cf` feat(player): Buffer size setting (Auto/64/128/256 MiB) clamped to free RAM
- `db03a2bc5` docs+feat(usb): pendrive preparo spec and read-only fs/ro probe
- `ebb5eb8fe` fix: diagnose USB cache rejections and expand mount discovery
- `3b12a5d2f` fix: preserve startup header in native log uploads
- `f170d77ba` fix(ajustes): say why P2P cache location and limit are greyed
- `1c9043ed0` fix(#551): recover stale profile sessions before retrying discovery
- `cbcf467f5` fix: parse stream hostnames before requiring P2P (#552)
- `26b9c635b` fix(fileiras): undeclared catalogs no longer take Home slots (#545)
- `a2e9daf91` fix(#551): open the profile session when a PIN completes the choice
- `6d524f947` fix: offer notWebReady streams that carry an http(s) url with P2P off (#552)
- `0c2a1ba2c` fix(player): remember the hand-picked audio track for the whole show
- `c2e50a932` homebrew: repo.json for 2.0.6
- `d570cb3c1` Tests: cover clock focus, page boundaries and remote input ownership
- `a7d8b1f58` Navigation: reach the clock with Up from browsing pages
- `bbb74484e` test(artwork): cover recommendation upgrades and Metahub quality
- `946ad1819` Artwork: upgrade shallow recommendation backdrops at High quality
- `85562a66b` Tests: cover deleted addon rows and discovery boundaries
- `11e6e8535` Catalogs: remove uninstalled addon rows and owned collection sources
- `5f0ad732e` test: cover independent Continue Watching startup and profile lifecycles
- `cfc15aeab` Prioritize Continue Watching without blocking Home catalogs
- `5ae92423e` Tests: cover metadata portraits, identity and profile ownership
- `8bd065a36` Metadata: retain valid cast entries and distinct Unicode identities
- `a1eca619b` Metadata: keep addon cast portraits when TMDB is unavailable
- `1c3043479` test(colhdr): mock three collection header directions
