# Nuvio Legacy 2.0.7

Everything merged into `integracao/2.0.5` after the `v2.0.6` tag (base `ae9c3c138`), plus the owner requests
that landed in the same window. Last tracking sync: 2026-10-18T04:47:55-03:00. Community server:
**https://discord.gg/9NWr6SHyzJ**

Release engineering is already merged on `agente/205-release207` (`b72189c5d`): version bumped to 2.0.7 in
`deploy/app/appinfo.json` and `tools/tizen-config.xml`, the what's-new card (`src/novidades_cartao.c`) points
at 2.0.7 and `docs/releases/2.0.7/NOTAS.md` holds the GitHub release notes. Before tagging, `repo.json` still
needs the built .ipk (url, sha256, size) and the frost performance line below has to be filled in.

> The in-app card (`src/novidades/207.inc`) is a shorter owner-curated list; this file is the complete release feature source.

---

## Release features (English)

### Highlights

- **Stage rows show details beside the poster**: Focused Stage posters reveal genres, ratings, synopsis and cast beside the artwork, with an optional spotlight that follows focus. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `a7d5ccb51`, `2dc2e5dc2`, `0749d883e`, `f59c70b50`, `96457c78a`. Stage now includes anchored headlines, timed focus motion and editorial lighting; touch scrolling and viewport bounds remain aligned.

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
- **Specials (season 0) are shown and playable** (#556, PR #569): anime with a special or out-of-order season
  that Trakt recognises are no longer dropped; the provider's episode numbering survives through the source,
  resume, next episode and Trakt/Simkl/Plex, and the fake per-episode season tab an add-on creates ("The
  Blacklist: Season 20+") is gone. All platforms. Refs #556, PR #569, `8dff0e2a9`, `4a443422b`, `94d16f7de`,
  `288e531ee`, `054ccef24`.
- **Frosted glass is cheaper and looks the same** (owner request): the glass blur now works only on the
  rects the panels read and at most at 960x540, with 3 passes instead of 5 and no stale recapture after a hard
  cut. On the C9 Home at 1080p the blur went from 498,960 to 24,289 pixels per frame and all twelve reference
  shots stay within 4 levels. All platforms. Refs owner request, `4c82273d7`, `93b3d69ec`, `3dc70083c`,
  `013e1341f`, `fe6a04804`, `4dba47b60`.
- **Polished friends row, no "See all"/"+ Add"** (owner request): all six friends row styles now use the
  standard Home focus (no red outline), landscape posters with text in the right place, an initial-letter
  avatar when there is no photo, a fallback poster when there is no art, and the two row buttons are gone.
  All platforms. Refs owner request, `8806e6aff`, `d95bd32e1`, `e27514693`, `f21d993b2`.
- **Mark as watched works** (#548): marking a whole series or one episode sticks after an older whole-series
  job or retry answers later, and confirmed marks survive a restart and a stale Trakt pull. All platforms.
  Refs #548, `66a608ef7`, `9bc31cc9d`, `6db247078`, `8330673a8`, `2d04fbecc`, `b074d9fd5`, `91da5935e`.
- **Player opening screen** (owner request): the logo slides to the centre and the Play pill morphs into a thin
  progress ring below it, over sharp artwork with a dark veil rising to half the screen; "Arte" is the new
  default, with "Arte borrada" and "Desligado" kept. The new **Estilo** setting picks **Centro (A3)** or
  **Canto (B3)**, logo bottom-left with a thin ring under it. All platforms. Refs owner request,
  `718e61abb`, `ace1fd53a`, `922500ef4`, `9c3ecdf85`.
- **Fixed Gray or Colour background in the Dynamic layout** (owner request): the Dynamic ("Apple TV")
  layout background is no longer art-only — Gray and Colour are now options, picked with the accent palette
  picker the settings already used, with a calmer Immersive mode. All platforms.
  Refs owner request, `6da1fc418`, `e91329dd2`, `0cf805980`, `e3fdbe717`.
- **Sidebar frost and Home frost islands** (owner request): the sidebar frosts over screens that carry
  glass, the accent pill becomes tinted glass in glass mode, the frost islands are no longer drawn under the
  Home sidebar glass, and modals only capture the region the panel reads — including the "Saved" modal.
  All platforms. Refs owner request, `90b0f9cd7`, `61764f975`, `fb4af3ee5`, `f087f4b59`, `7bf4caac9`,
  `e3eb56d39`, `37687dce0`, `90c8d380d`, `ab0e9a597`.
- **Sources sheet grows from the clock**: the sources sheet now opens out of the clock island, with the same
  spring and material as the rest of the interface. All platforms. `fbe06f5e7`, `ae39b748a`.
- **See all grid** (PR #568 #563): showcase rows in collections and the search results of each catalog open
  into a grid of every item, without the row limit. All platforms. Refs PR #568 #563, `4baf783fa`,
  `e0ddeca5f`.
- **Catalog outside the quota refreshes right away** (#545 follow-up): a row you just added renews on its own,
  without needing another action. All platforms. `975638744`, `53b050753`, `b03502f84`.
- **Search and episodes work again** (#555): exact-title series like "Bigg Boss" are no longer cut by the
  result cap, and series that come from an add-on with no episodes get their list filled from the catalog.
  All platforms. Refs #555, `2e5b2ffde`, `d479a8463`.
- **Catalogs: ghost rows and vanishing sources** (#545 #551 #552): rows of uninstalled add-ons leave Home, rows
  the add-on does not declare this session no longer take a slot, and sources come back when the add-on probe
  is slow. All platforms. Refs #545 #551 #552 #504, `26b9c635b`, `1c9043ed0`, `455135df2`, `cbcf467f5`, `27988615e`.

### New

- **Clearer title pages with adjustable spacing**: Tint-only cast, trailer and rating cards share the page backdrop, while Compact, Normal and Comfortable row spacing keeps ratings, seasons and cast aligned at different text sizes. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `b8e94cf5c`, `5a3f41c11`, `83177ade2`, `79eca720f`, `e0c9ba77b`.

- **Optional accent focus plate**: Appearance offers an accent-colour focus plate, including spotlight and profile rows, disabled by default. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `09e9a00ba`, `cc0222525`.
- **Smoother Sources tabs**: Sources drops its panel shadow and slides the selected tab while fading lists, loading messages and errors, respecting reduced animation. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `fe6ebfd6d`, `e7a84b249`.

- **Context menu keeps Home visible**: Home stays behind the context menu, and the enriched poster card is configurable. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `d8e4be39c`, `4759f46fd`.

- **Row title styles**: *Settings › Home › Row titles* picks Classic, Island (title in a glass pill with
  the row icon and item count), Editorial (small uppercase kicker above the spaced title) or Accent (a dot
  that grows into a bar on the focused row, with a "See all" tail). All platforms.
  Refs owner request, `331746034`, `4898972d7`.
- **Arabic in Noto Naskh Bold** (community request): the Arabic UI and plain Arabic subtitles now use Noto
  Naskh Bold at every weight instead of synthetic bold, with the baseline aligned to Latin text; Latin, CJK
  and Thai are untouched. All platforms. Refs community request, `458828591`, `0940ae7bf`.
- **Big poster row in three sizes**: the info-on-poster row gains Small, Medium and Large
  (*Settings › Home appearance › Row style*, left/right like Landscape and 4:3) — Small with one synopsis
  line, Large with three and the spare height turned into width. Medium is unchanged.
  All platforms. Refs owner request, `e50a38ef6`, `cbf6fb021`.
- **Smoked glass over the video** (*superseded*): this is no longer the final look — see "Real glass back
  over live video" in Fixes. All platforms.
  Refs owner request, `6f1c61cb8`, `a691e96fb`.
- **Automatic glass quality**: *Settings › Glass quality* (Automatic, Performance, High) — Performance uses
  only the smoked glass, with no capture or blur; Automatic drops to smoked by itself after 4 consecutive
  150-frame windows over the target frame time. All platforms.
  Refs owner request, `30a932909`, `a691e96fb`.
- **Dolby Vision opening in the new style**: the Dolby Vision Cinema screen now uses the same opening as the
  other loading screens, with Style (Centre/Corner), Background (Off/Art/Blurred), the add-on line, the
  detailed card and reduced animations all applying the same way. All platforms.
  Refs owner request, `d89784e82`, `8f788b25b`, `b59987511`, `ef33525a8`, `4542aeffe`, `abb9e49b9`.
- **Frost in the player and its menus**: the buttons, the control bar, the source/subtitle/audio panels,
  the live Statistics panel and the quick menu / parental guide now use the same frosted glass as the rest
  of the interface instead of a flat panel. All platforms (where the video can be read by GL).
  Refs owner requests, `46e0bd5bd`, `3982ef84b`.
- **Translucent top chrome with frost off**: with Glass on and "Vidro fosco" off, the top bar (and the
  avatar/clock island) is see-through tinted glass instead of opaque. LG webOS. `b5007ae85`.
- **Home follows your changes without a restart** (#468, owner request): reordering or disabling rows in
  Settings and adding a new Jellyfin server/add-on refreshes Home on the spot, and a server with many
  libraries stops showing only four catalogs — the view cap went from 6 to 16, boxsets and untyped views
  are accepted. All platforms. Refs #468, `b0e429762`, `c47a40788`, `08c98b017`.
- **One less button in the player** (owner request): the previous-episode button leaves the series action
  bar; next episode and every other button stay, with focus order and spacing intact. All platforms.
  Refs owner request, `4e6b592d8`, `d51d4e46c`.
- **"P2P cache" row on Android**: the USB drive cache setting shows up and works in the standard Android
  APK, as it already did on webOS. Android. `9b4f37efb`.
- **See all grid in collections**: showcase rows open into a full "See all" grid of the collection.
  All platforms. `4baf783fa`.
- **Exact episode identity in scrobbles**: progress sent to Trakt uses the exact episode identity.
  All platforms. Refs PR #560, `ff1f1aa25`.
- **Series identity kept through lookup and playback** (#524, PR #561): a series stays the same series while it
  is looked up and played, instead of turning into a different title, and the requested episode is the one that
  plays — this covers Continue Watching and the resume cards too. All platforms. Refs #524, PR #561,
  `cdf5bdd88`, `2537db6a0`, `86d194462`.
- **Continue Watching up to 40 items** (PR #538): the native cap went from 12 to 40 and future rows are
  published up to it. All platforms. Refs PR #538, `4cbf5ad5c`, `696a3a6ee`, `cee137de7`.
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

- **Library and catalog tabs are real glass** (owner request): the tab strips (Saved/Collection/Lists,
  Movies/Series, the title pill and the remaining segmented controls) now use the same frosted glass as the
  rest of the app, with the selected tab keeping its accent fill. All platforms. Refs owner request,
  `21696fd8c`, `796a3441f`, `a096e5eb8`.
- **Settings at 100% by default** (owner request): the Settings scale ships at 100% instead of 80%; anyone who
  never picked a size by hand moves to 100% and an explicit choice stays as it was. All platforms.
  `3f8d304b7`, `f22761e28`.
- **High-quality artwork is fetched at the size it needs** (owner requests): at High quality, w342 posters and
  w780 backdrops stored by the catalog are fetched at the size the draw needs, and the 480px small backdrop
  is no longer served for cards up to 640px. All platforms. `ff1cd6dcd`, `44bf4884a`.
- **Long poster URL templates** (owner requests): pasting a long poster URL template on the phone page is no
  longer cut at 1024 characters, and the native keyboards accept the same limit. All platforms. `0950a8eb8`,
  `df7c93aaf`.
- **MKV audio track remembered**: on MKV with a converted track, the hand pick keeps sticking for the next
  episode, even with two identical tracks. All platforms. `3486c616b`.

### Fixes

- **Discord activity keeps public artwork**: Unsupported poster URLs are converted to public Discord artwork URLs so shared activity can display its image. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `115220cef`, `e476d1707`.

- **Player controls stay translucent**: Video controls, post-play cards, error panels, PiP close and hints respect glass opacity even with glass disabled, while other screens retain their normal surfaces. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `aafbe8739`, `d2ebbd9bd`, `5331d4239`, `e272873d1`.
- **Watched action stays on the selected title (#595)**: Switching titles cannot send a queued watched action to the wrong title in Trakt history. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `4c7d236ba`.
- **Reliable Trakt Up Next (#595)**: Confirmed next episodes remain available when metadata is missing or stale. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `c78f36b02`.
- **Profile artwork survives cache pressure**: The profile picker retains artwork between frames and retries loading when a safe cache slot becomes available. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `f5eb11656`.
- **Clear subtitle-sync feedback**: Subtitle sync explains when no embedded subtitle exists for comparison. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `43d758ded`.
- **Intro markers use the current service**: TheIntroDB v4 requests preserve title addressing and provider episode numbering without reusing obsolete negative-cache results. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `66101da36`.
- **Text scaling preserves artwork**: Search, keyboards, episode overlays, collection rows and View all keep artwork dimensions and navigation aligned when text size changes. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `8b1f605d0`, `dab8872c2`, `9dbe1a156`, `ef1b092c4`, `ebb0b1365`, `e24e614a2`, `5ff1d7ef3`, `a22f0dac1`, `f04e6de29`, `93fb92118`, `79a2ec3e0`, `1eb28a937`.

- **Cleaner player panel headings**: Subtitle, audio and episode panels start at their headings without a duplicate small title line. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `0a3eacf26`, `6660e72a8`.

- **Friends rows respect screen edges (#205)**: Friends rows clip to the real screen width when text scaling is enabled. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `ae8d8486d`, `1a3e722bd`.

- **Search keyboard spacing and symbols**: Search gains corrected menu spacing, a restyled keyboard and a dedicated ?!# special-character page. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `f1b3b5e67`, `bc76ee8bc`.

- **IMDb score no longer shown as %** (#590): MDBList sometimes sends the IMDb rating on a 0-100 scale
  and the app displayed "80%"/"80.0"; now only the IMDb value is divided by 10 (8.0), leaving Trakt and
  Rotten Tomatoes untouched. Platforms: Samsung .tpk, Android. Refs `03a144294`, `0d998e33a`, `eea9aac9d`.
- **CNCVerse live channels play again**: long add-on ids (cnc:…, 72+ bytes) were dropped or cut, so
  /stream/ went out truncated and returned 0 sources; ids now travel whole via a short stable alias.
  All platforms. Refs `630292d7e`, `47ad499ad`.
- **Smoother Home trailer transitions**: the trailer cutout fades as Frost returns, avoiding an abrupt backdrop change. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `524de66ac`, `240bc60a0`.
- **Cleaner shuffle focus**: the detail-page shuffle button no longer displays an unnecessary focus tooltip. Platforms: webOS, Android, Samsung .tpk, WASM. Ref `c9c2d3f15`.
- **Cast identities stay distinct**: valid cast entries survive metadata filtering and different Unicode names retain separate identities. Platforms: webOS, Android, Samsung .tpk, WASM. Ref `8bd065a36`.

- **Dynamic background no longer flickers**: with frosted glass on in the Dinâmica layout the deferred
  background is resolved before the frost backdrop copy, so Home stops alternating between the right art
  and colour bands on Android. Android. Refs owner request, `5528dc0ad`, `6a64c2e13`.
- **Split hero without a repeated title**: in the split hero the poster list no longer repeats the title
  that is already in focus. All platforms. Refs owner request, `4428dea18`, `f41a4e154`.
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
- **Real glass back over live video** (owner request): the player and the panels over the picture blur for
  real again — true translucent glass instead of the smoked slab — with a denser tint and a governor that
  trials once instead of dropping the whole app. All platforms.
  Refs owner request, `ce33fe44a`, `6974f7293`, `db42a0a55`, `0fb1a0b1c`, `04375ab4f`.
- **Focused episode text in a padded glass plate** (owner request): on a series page the focused episode's
  title/synopsis plate now spans the card, pads its text and uses the same glass as the other panels
  (dropping to smoked when quality degrades). All platforms.
  Refs owner request, `df06809f2`, `3b39999e5`.
- **C9 screen no longer blinks with frost on** (owner request): hero-promoted artwork was losing the mipmaps
  the rows also draw — every frame rebuilt the chain on the main thread; the mipmaps now stay.
  LG webOS. Refs owner request, `98bc829e1`, `fc6d35d69`.
- **Many Jellyfin catalogs show up** (#468): servers like AIOMetadata expose one view per catalog; the app
  stopped at 6 and only accepted movies/tvshows, so about four catalogs appeared. The cap went to 16 views,
  with boxsets, mixed and untyped views accepted, and the item budget is split across rows so memory does
  not grow. All platforms. `b0e429762`, `c47a40788`.
- **Kitsu special season without breaking the source**: when the Kitsu add-on does not report a season it
  stays absent instead of becoming 0 — the provider's numbering keeps working in the source, on resume and on
  the next episode. All platforms. `4a443422b`.

### Performance

- **Glass stays smooth when text caches change**: Retiring unrelated textures no longer rebuilds an unchanged glass backdrop, preserving the existing frost appearance. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `3e8becc0e`, `59efaef75`.

- **Playback buffers adapt to demanding streams**: Android sizes buffers to bitrate within memory limits and can try another available source after sustained throughput problems, with corrected policy handoff and prepared buffers. Platforms: Android. Refs `0d3e01a5f`, `c91106066`.
- **Stronger low-memory relief (#415)**: Playback cache limits follow available memory, release decoded buffers under pressure and gate retries after pipeline errors. Platforms: webOS. Refs `87977f537`, `370769c09`, `bcdd3eb04`, `f77eeeef7`.

- **Smoother title-to-episode transition**: Detail frost no longer re-blurs its backdrop every frame while moving between title and episodes. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `fcd6c995b`, `f420d6d0e`.

- **Memory relief on 2 GB webOS TVs (#415)**: Memory pressure flushes subtitle caches, reduces PiP memory floors and caps playback allocations using available memory. Platforms: webOS. Refs `8feec1a91`, `549e2273c`.

- **Frost only on the rects the panels read**: with the Dynamic background deferred, the backdrop copy is
  resolved once per hash change instead of per frame. All platforms. `5528dc0ad`.
- **Much lighter Home and glass on Android (TCL) and .tpk**: one backdrop capture per frame instead of
  4 copies and 20 blur passes (4.00 captures / 20 passes before, 1.15 / 5.75 now). Android, Samsung .tpk.
  `0c77460c1`, `d98d72026`, `5536ee8da`, `d7ec89975`.
- **Continue Watching no longer blocks Home**: catalogs open while the row is built. All platforms.
  PR #538, `cee137de7`, `cfc15aeab`, `8c99638f7`.
- **Buffer clamp accounts for active DTS**: the in-use range reserves its 4 MiB before queue capacity is
  chosen. All platforms. `62421bfdc`.
- **Frosted glass: regional blur and 3 passes instead of 5**: only the rects the panels read are blurred, the
  target stops at 960x540 (480x270 in one pass) and each target is cleared before its pass; a hard cut recaptures
  on the same frame instead of waiting. Animated/focused C9 Home at 1080p: 498,960 → 24,289 blurred pixels per
  frame; at 4K 1,995,840 → 56,621. Extra cost: one full RGB texture saved per layer (5.93 MiB at 1080p).
  All platforms. `93b3d69ec`, `3dc70083c`, `a0f765d7f`, `17717812d`, `013e1341f`, `fe6a04804`, `4ae3e7211`.
- **Glass interior skips the rim math**: most of a big panel used to return after computing the edge normal and
  rim weights it never uses; the frosted path solves density and returns first. Same output. All platforms.
  `4dba47b60`.
- **Frosted glass from pre-blurred art** — *reverted in this window*: the glass blur started reading a half-res
  copy of the backdrop layer alone (1.15 → 0.35 captures per frame), but on the TCL the owner found the previous
  look prettier and lighter and the merge was undone (`f40a93dc3`); the optimization of the current path landed
  instead (`4c82273d7`). All platforms. `e9c809475`, `ab02d4aaa` (the path that left).
- **Home reacts to reordering/enabling rows without reloading everything**: the network refresh that follows
  closing Settings runs once, and server rows over the Home cap are clamped instead of dropped. All platforms.
  `b0e429762`.
- **Detail: fewer full-screen layers**: the trailer card's hole and veil are drawn in one pass, and a
  neighbour card almost off screen no longer counts as full — 4 → 2 layers, fill 1.81 → 0.99 on the harness,
  PNGs byte-identical. All platforms. `5ed6ddfaa`.
- **Changing Home rows copies far less** (owner request): while the hero crossfades, the glass copied the
  whole screen just to blur a few rectangles; it now copies only those rectangles — copy cost per capture
  drops from about 1.0 to 0.13 screens. It does not fix the idle 40 fps or the 4.7x fill spikes on the TCL
  (that needs on-device GPU timing). All platforms. Refs owner request, `e70adbf7a`, `d7522dd17`.
- **Title → season tabs no longer stutters** (owner request): the transition that hit 71.8 ms/frame on the
  TCL with 163 blur rects now grows cheap glass and culling; and the preferred source stops being matched
  every frame (the `[fonte]` log alone wrote 2,347 lines in 2 min) — computed once per list, logged once.
  All platforms. Refs owner request, `df06809f2`, `e5eb23eba`, `3aec9a8c0`, `3b39999e5`.
- **Faster source opening on Android** (@Tolu-Walop-E, PR #572): the end-of-file index is read over the
  parallel connections instead of a serial probe, later requests open straight at the add-on's known
  destination, and a remembered destination that serves another file is no longer trusted — the stream
  starts sooner without a wrong probe. Android. Refs PR #572, `cc61bfdd8`, `809bd7c96`, `8876c13c6`,
  `f6618db6b`, `22f3d22bb`.
- **Trakt comment cards cost zero glass-shader pixels**: up to 8 detail cards of 600x340 each took their own
  frosted read (~0.80M glass-shader px/frame); they now use the flat chip material with a top highlight —
  0 glass-shader px, rim and highlight still visible. All platforms. Refs `bb9f2abc9`, `d4317aaa4`,
  `24047051b`.
- **No full-screen blur when glass is off**: the zero-region fallback copied and blurred the whole screen
  on every backdrop change even with the glass interface disabled; the fallback is now gated on
  `ajustes_vidro()`, and when a frame has no glass the screen is blurred only once. All platforms.
  Refs `033c3fa70`, `0723edb59`, `3f95a9241`.

### Community

- **Automatic TV and Mobile mode on Android** — @community, PR #431 (`c305e6f7e`, `875d7f18f`).
- **Series identity during lookup and playback** — PR #561 (`cdf5bdd88`, `2537db6a0`, `86d194462`).
- **Continue Watching without blocking catalogs** — @ghbarker, PR #538 (`cfc15aeab`, `a748ebf32`).
- **Keep add-on cast portraits when TMDB fails** — @ghbarker, PR #540 (`a1eca619b`, `8cc5d723d`).
- **Remove uninstalled add-on rows** — @ghbarker, PR #543 (`11e6e8535`, `0d44804f0`).
- **Full recommendation backdrop and artwork quality** — @ghbarker, PR #546 (`946ad1819`, `66299e5f0`).
- **Reach the clock and resume island with Up** — @ghbarker, PR #547 (`a7d8b1f58`, `960ec872a`).
- **Season 0 Specials kept and playable** — @ghbarker, PR #569 (`8dff0e2a9`, `054ccef24`).
- **Remember the hand-picked audio track** — @Tolu-Walop-E, PR #550 (`0c2a1ba2c`, `2cec223fd`).
- **A direct URL plays without P2P** — @mark-major, PR #559 (`d25a3e513`, `06724b3ed`).
- **Faster source opening on Android** — @Tolu-Walop-E, PR #572 (`cc61bfdd8`, `809bd7c96`, `8876c13c6`,
  `f6618db6b`, `22f3d22bb`).

### Internal

- **C9 rendering and subtitle timing checks**: Regression checks cover glass-cache budgets and unchanged frost images, plus production readiness, the complete initial 12-second ASS window, and deterministic prefetch timeout checks. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `5f29a8095`, `b12b8e11f`, `9e778e588`, `3934284c5`, `8e8a90415`, `0661e272d`.

- **Broader visual regression coverage**: Sources shadows, profile-cache pressure and deterministic text-scale fixtures cover more UI states. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `45aef45f9`, `915e360a4`, `d6b0c2363`, `cff9c7e1b`, `15903519f`, `0c0c80d58`, `0d5d0d735`, `04ed0d150`, `207e0a283`.
- **Tracking and release documentation**: Release notes, issue tracking and fixed-width caller documentation stay synchronized. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `733bc65e0`, `442021716`, `0eee6cbc3`, `71a57d7b4`.

- **Graphics regression coverage**: frame-hash, native trailer and ANGLE GLES2 probes cover Home artwork flicker; measured Frost and menu pixel checks replace misleading fixtures. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `491f8ce0e`, `698fa7417`, `0f1b20514`, `01603fd5b`, `453c7a73c`, `fff6df801`.
- **Integration fixture repairs**: isolated Mach-O linking, discovery retry, sync handshakes and fallback artwork fixtures remain runnable. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `5b37cecd8`, `ac616c0bd`, `da2c69701`, `f87c41ed7`.
- **Release and provider coverage**: the richer translated release card and PenguPlay fixtures document the merged behavior. Platforms: webOS, Android, Samsung .tpk, WASM. Refs `1c1dfa50e`, `a779b3dcd`, `c32a87bbf`, `68cf2c63b`.

- New tests: `tests/dvabertura_shot.sh` (FAIL before, PASS after) for the DV opening; rows with the three
  sizes on one line, the enum appended at the end and help text in both catalog and collection; monotonic
  geometry of the big poster in both layouts; PNGs of the three sizes in `pgtam_shot` (`e50a38ef6`,
  `331746034`).
- New tests for the Dynamic background flicker (`tests/dina_pisca_shot.sh`) and the split hero (last items
  of the row, no wrap) (`5528dc0ad`, `4428dea18`).
- Smoked glass: the test now requires zero captures and zero blur passes and counts the smoked glasses; the
  baked stand-in blur source in the player was dropped (`6f1c61cb8`).
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
- New `tests/homerefresh.sh`: Home follows row reorder/disable and new sources without a restart; home and
  collection fixtures repaired after a merge (`c47a40788`, `0bfe5ce71`).
- 1080p frost pixel-difference limit pinned in a test: all twelve C9 PNGs against the old path, masking only
  the live clock digits (`4ae3e7211`); typed-title and late-playback identity tests (`2537db6a0`); an assert
  that friends row style 5 has no "+ Add" column (`d95bd32e1`) and that its touch scroll covers the last
  friend's expanded panel (`e27514693`).
- Settings scale migration to a 100% factory default, only for anyone who never chose by hand
  (`3f8d304b7`), and a pixel proof that the Library/catalog tabs carry regional frost
  (`21696fd8c`, `796a3441f`).
- The `tests/homerefresh.sh` fixture repaired again at merge time: duplicate home stubs and missing add-on
  lock stubs (`c47a40788`).
- Glass merge-window regression closed: `tests/vidro_qualidade.sh` failed at `0940ae7bf`
  ("Alta ignores the governor (frost)"); the fix (`0723edb59`, whole-screen blur once when no glass last
  frame, plus the `ajustes_vidro()` gate `033c3fa70`) is merged via `agente/205-vidroqual` (`3f95a9241`).
- New tests for Naskh Bold (`tests/arabe_negrito_padrao.sh`, FAIL without the `text.c` change) and for the
  Home backdrop copy cost (`tests/homelerdo.sh`, 5 checks, FAIL on the baseline) (`458828591`, `e70adbf7a`).
- New Trakt-comment tests (`tests/traktcoment.sh`, series/episode fixtures, a 180-frame per-frame counter and
  the FAIL->PASS for the series regression), the shared wrap cache for that row and the sources-sheet
  generation token / "Play from beginning" (`5195cd14f`, `1165245fc`, `075fec9a4`, `6a6d5b71b`).

### Pending (not integrated yet)

Work on open branches with no merge — **not** part of 2.0.7 as it stands:


- **PR #487 v2 by @ghbarker** (add-on reported audio and subtitle languages; `e4fb8c08`) — `agente/205-pr487v2`, 3 commits out,
  VEREDITO OK `j-rev-pr487-v2`, no commit after. **Needs final re-review.** Android, LG webOS.
- **PR #493 v2 / v3** (profile and session) — `agente/205-pr493`, 12 commits out. VEREDITO **BLOQUEIA**
  `j-revg-pr493` (newer); the OK `j-revpr493-v3` came before. Needs a new fix + review.
  Android, LG webOS.
- **Cheap frost identical to the one approved on the LG C9** — `agente/205-foscovelho` was **merged**
  (`76bc9d0c3`) and then **reverted** (`f40a93dc3`): on the TCL the owner found the previous frosted glass
  (screen capture + blur) prettier and lighter. The pre-blurred art source (`ab02d4aaa`, `d0789c13a`,
  `50a1ac232`, `35f5f4fc3`) is **not** in 2.0.7; its round-4 review was OK (`j-rev-foscovelho-r4`, 22:08),
  already after the merge. The path that shipped — the screen capture, just cheaper — landed as `4c82273d7`
  and is under Highlights/Performance. All platforms.
- **Detail "more" menu on glass** — `agente/205-detalheglass` was **merged** (`cc87bec32`, `e047eecee`, `9ebdc9a3b`, 10:34) after the
  fixes `0b52916d0`, `d04f21ba4`, `531a36f6e`, `937a69475`, `26dbd9a2a`, `3aec9a8c0`, `aef4e3c58`; the
  rev4 BLOQUEIA review (09:20) came **before** the merge. It is under Highlights now; the merged result
  still needs a review. All platforms.
- **Sidebar frost and glass accent pill** — `agente/205-vidromenu` was **merged** (`ab0e9a597`, 08:11,
  conflicts resolved by the merge) and `agente/205-vidroilhas` was **merged** (`f087f4b59`); both are now
  under Highlights. The rev2 review was OK (`j-rev2-vidromenu`, 06:49) **before** the merge, so the merged
  result still needs a review. All platforms.
- **Fixed Gray/Colour background in the Dynamic layout** — `agente/205-dinfundo` was **merged**
  (`e3fdbe717`, 07:49); rev2 review OK (06:48) after the accent-palette fix (`0cf805980`). It is now under
  Highlights.
- **Text and component scale** — `agente/205-textoescala`, 19 commits out (`e7fa6d67c`, `406359d3a`,
  `bab9c56e3`, `86fed77d7`, `4bfafc814`, `342aafc49`, `54242990a` and others): a new "text and component scale" setting (85–130%) that scales
  text, buttons and components while leaving poster/card sizes alone. **Review BLOQUEIA (06:50)**: the
  option never shows up in Settings › Appearance (P1), cards still resize (P2) and Library rows overlap at
  85% (P2). Owner request. All platforms.
- **Trakt comments with glass, frosted context menu and #562/PR #574** landed in this window
  (`335bd2abc`, `a363d821c`, `dc040ef2d`, `a2ea074c7`, `2c7c85860`, all review OK: `rev-traktcoment`, `rev-ctxgrande`, `rev-pr574`) and
  are already in New/Fixes/Performance. Still to measure on a device: the FPS of the 8 glass panels.
- **Cheaper frost, polished friends row, PR #561 (#524), glass tabs and Settings at 100%** landed in this
  window (`4c82273d7`, `f21d993b2`, `86d194462`, `a096e5eb8`, `f22761e28`) and are already under
  Highlights/New:
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

### Merged in this window (not pending any more)

Landing in 2.0.7, already in the sections above:

- **A3 loading screen** (`ace1fd53a`), **watch tracking / ghost seasons** (`91da5935e`, `c0dc799a3`),
  **#545 quota follow-up** (`b03502f84`), **deep links** (`a4f220cd7`), **external player** (`281b4af37`,
  `161cdebd2`), **PR #569 Specials** (`054ccef24`), **#555 search/episodes** (`d5ae66d9e`, `438423097`),
  **#551** (`e680776ed`), **#552** (`0d7821a01`), **#558** (`a1888d007`), **#483** (`251632be`),
  **#529** (`f8fbd3bd3`), **row styles / See all** (`4baf783fa`, `d2b4e0b12`, `76c6e8eea`),
  **Vidaa** (`d0bf5ba79`), **collection header** (`fec2dc0ac`), **PR #431 TV/Mobile auto** (`875d7f18f`),
  **USB P2P on every platform** (`0fe8a7731`, `2ed922f03`, `84c1a90b6`), **buffer size** (`35a944d9f`),
  **trailer audio + captions** (`b43655122`, `31dcf5275`), **P2P reasons** (`e21441cd2`, `30b57ccf1`),
  **glass/frost work** (`46e0bd5bd`, `b5007ae85`, `e9c809475`, `0c77460c1`, `d98d72026`),
  **release 2.0.7 bump and notes** (`b72189c5d`, `e275de18a`),
  **#468 Home/Jellyfin views** (`08c98b017`), **previous-episode button removed** (`d51d4e46c`), the
  **opening-screen style setting A3/B3** (`922500ef4`, `9c3ecdf85`), the
  **revert of the cached-art frost** (`f40a93dc3`, undoing `76bc9d0c3`), the
  **cheaper screen-capture frost** (`4c82273d7`), the **polished friends row** (`f21d993b2`), **PR #561**
  (`86d194462`), **glass tabs** (`a096e5eb8`), **Settings at 100%** (`f22761e28`), the **Trakt comments row**
  (`335bd2abc`), the **frosted context menu + Poster menu setting** (`dc040ef2d`), **PR #574**
  (`2c7c85860`), the **Dynamic Gray/Colour background** (`e3fdbe717`), the **Home frost islands**
  (`f087f4b59`), the **sidebar frost / glass accent pill** (`ab0e9a597`), **PR #572** (`22f3d22bb`), the
  **player glass / episode plate / hero mipmap** fixes (`04375ab4f`, `3b39999e5`, `fc6d35d69`), the
  **region-only backdrop copy** (`d7522dd17`) and **Arabic Naskh Bold** (`0940ae7bf`) and the **IMDb 0-100 fix** (`eea9aac9d`), the **flat-glass Trakt cards** (`24047051b`), the **governor/glass-off fixes** (`3f95a9241`) and **CNCVerse long ids** (`47ad499ad`), the
  **text/component scale merge** (`9f0588a80`, `302d2bf4f`, `b0c947387`), the **#557 seek warning**
  (`5cf77f08d`, `d034960e8`), the **#581 audio-focus fix** (`5b0d2a35d`, `4a0b3408d`), the
  **#584 720p Auto trailers** (`2bc1ddb55`, `b89ddc139`) and the **#589 slow-motion GIF**
  (`5ab9fc44c`, `b2aa59ada`).

### Frost performance (to fill before tagging)

Placeholder carried from `j-release207`: the measured numbers belong here once the frost work is measured on a
device. Points to report: one backdrop capture per frame on Android/.tpk (`0c77460c1`, `d98d72026`, `5536ee8da`)
and one capture per backdrop change everywhere (`e9c809475`).

**Measured (`4c82273d7`, `/Volumes/ExternalSSD/tmp/foscoleve3/medidas.md`):** blur pixels per frame on the
animated/focused C9 Home at 1080p went from **498,960 to 24,289** (native/webOS and Android), at 4K from
**1,995,840 to 56,621**; passes per capture 5 → 3 (4 at 4K); Explore 47,520/59,400 → 11,394/14,243; Settings,
detail and search keep **0 captures** when static and unchanged. Captures per frame and the copied screen
fraction stay the same as the old path (0.35). All twelve C9 1080p reference shots pass with a maximum
8-bit channel difference of **4** against the old path (limit 5), masking only live clock digits. The
every-other-frame throttle was dropped: a changed draw hash always refreshes that frame. Extra cost: one full
RGB backdrop texture per layer — **5.93 MiB at 1080p, 23.73 MiB at 4K** — and a region miss may blur the
saved backdrop on that frame. No TV GPU timing yet; these are Mac-side counters.

### Merged in this window (not pending any more)

Landing in 2.0.7, already in the sections above:

- **A3 loading screen** (`ace1fd53a`), **watch tracking / ghost seasons** (`91da5935e`, `c0dc799a3`),
  **#545 quota follow-up** (`b03502f84`), **deep links** (`a4f220cd7`), **external player** (`281b4af37`,
  `161cdebd2`), **PR #569 Specials** (`054ccef24`), **#555 search/episodes** (`d5ae66d9e`, `438423097`),
  **#551** (`e680776ed`), **#552** (`0d7821a01`), **#558** (`a1888d007`), **#483** (`251632be`),
  **#529** (`f8fbd3bd3`), **row styles / See all** (`4baf783fa`, `d2b4e0b12`, `76c6e8eea`),
  **Vidaa** (`d0bf5ba79`), **collection header** (`fec2dc0ac`), **PR #431 TV/Mobile auto** (`875d7f18f`),
  **USB P2P on every platform** (`0fe8a7731`, `2ed922f03`, `84c1a90b6`), **buffer size** (`35a944d9f`),
  **trailer audio + captions** (`b43655122`, `31dcf5275`), **P2P reasons** (`e21441cd2`, `30b57ccf1`),
  **glass/frost work** (`46e0bd5bd`, `b5007ae85`, `e9c809475`, `0c77460c1`, `d98d72026`),
  **release 2.0.7 bump and notes** (`b72189c5d`, `e275de18a`),
  **#468 Home/Jellyfin views** (`08c98b017`), **previous-episode button removed** (`d51d4e46c`), the
  **opening-screen style setting A3/B3** (`922500ef4`, `9c3ecdf85`), the
  **revert of the cached-art frost** (`f40a93dc3`, undoing `76bc9d0c3`), the
  **cheaper screen-capture frost** (`4c82273d7`), the **polished friends row** (`f21d993b2`), **PR #561**
  (`86d194462`), **glass tabs** (`a096e5eb8`), **Settings at 100%** (`f22761e28`), the **Trakt comments row**
  (`335bd2abc`), the **frosted context menu + Poster menu setting** (`dc040ef2d`), **PR #574**
  (`2c7c85860`), the **Dynamic Gray/Colour background** (`e3fdbe717`), the **Home frost islands**
  (`f087f4b59`), the **sidebar frost / glass accent pill** (`ab0e9a597`), **PR #572** (`22f3d22bb`), the
  **player glass / episode plate / hero mipmap** fixes (`04375ab4f`, `3b39999e5`, `fc6d35d69`), the
  **region-only backdrop copy** (`d7522dd17`) and **Arabic Naskh Bold** (`0940ae7bf`) and the **IMDb 0-100 fix** (`eea9aac9d`), the **flat-glass Trakt cards** (`24047051b`), the **governor/glass-off fixes** (`3f95a9241`) and **CNCVerse long ids** (`47ad499ad`), the
  **text/component scale merge** (`9f0588a80`, `302d2bf4f`, `b0c947387`), the **#557 seek warning**
  (`5cf77f08d`, `d034960e8`), the **#581 audio-focus fix** (`5b0d2a35d`, `4a0b3408d`), the
  **#584 720p Auto trailers** (`2bc1ddb55`, `b89ddc139`) and the **#589 slow-motion GIF**
  (`5ab9fc44c`, `b2aa59ada`).

### Frost performance (to fill before tagging)

Placeholder carried from `j-release207`: the measured numbers belong here once the frost work is measured on a
device. Points to report: one backdrop capture per frame on Android/.tpk (`0c77460c1`, `d98d72026`,
`5536ee8da`) and one capture per backdrop change everywhere (`e9c809475`).

**Measured (`4c82273d7`, `/Volumes/ExternalSSD/tmp/foscoleve3/medidas.md`):** blur pixels per frame on the
animated/focused C9 Home at 1080p went from **498,960 to 24,289** (native/webOS and Android), at 4K from
**1,995,840 to 56,621**; passes per capture 5 → 3 (4 at 4K); Explore 47,520/59,400 → 11,394/14,243; Settings,
detail and search keep **0 captures** when static and unchanged. Captures per frame and the copied screen
fraction stay the same as the old path (0.35). All twelve C9 1080p reference shots pass with a maximum
8-bit channel difference of **4** against the old path (limit 5), masking only live clock digits. The
every-other-frame throttle was dropped: a changed draw hash always refreshes that frame. Extra cost: one full
RGB backdrop texture per layer — **5.93 MiB at 1080p, 23.73 MiB at 4K** — and a region miss may blur the
saved backdrop on that frame. No TV GPU timing yet; these are Mac-side counters.
