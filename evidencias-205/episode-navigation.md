# Modern episode row rendering

Branch: `agente/205-episodioslento`.

The glass rating layout calls `desenhaCartao(..., alpha=0)` to measure its height. Episode section position calculations repeat that measurement. `txt_desenhar_tom` submitted those invisible text quads anyway. The shared text renderer now skips alpha-zero draws while preserving measurement and cached line dimensions.

Episode thumbnails request a stable maximum focus width, preventing successive 32-pixel decode promotions during zoom. Episode culling keeps only the focus rim/zoom margin; hidden cards do not reach texture requests or text layout. Existing text, image and glass caches remain in use; no additional cache is needed for the invisible draws.

## Reproduction and results

`bash tests/detail_episode_perf.sh` renders the real Modern detail screen in a hidden OpenGL window/FBO, without network fixtures. It uses ten episodes with local thumbnails and glass enabled, a deterministic detail animation clock, a complete navigation pass, and a measurement-only draw check. The regression now compares episode rectangles and GPU fill directly against the cast-focused screen; the former 220-rectangle / 3.2-screen allowance is removed.

The earlier alpha-only check reported rc 0 against a weaker allowance; that result does not satisfy the current acceptance test. The strict test at `05426cdee` was reproduced with **rc 134**: 175 rectangles / 2.359 screens of fill against cast 169 / 1.610, with 4,630 of 5,120 navigation frames over budget and churn=0. Log: `/tmp/nv205-before.log`; captures: `/tmp/nv205-before-{episodes,ring-scroll,zoom-scroll}.png`. Static glass requires zero captures during the measured eight frames.

`episode-before-episodes.png` and `episode-after-episodes.png`: ImageMagick AE comparison **0 changed pixels**. The comparison isolates the alpha guard; episode geometry changes do not affect this initial, ring-focused capture.

Final build: `NUVIO_MAC_SO_COMPILAR=1 NUVIO_MAC_BIN=/tmp/nv205-final-mac bash tools/mac.sh > /tmp/nv205-final-build.log 2>&1`, **rc 0**. No rendering changes were retained from this follow-up.

## Limits

The cast-focused screen measures 169 rectangles / 1.610 screens of fill: exact cast parity is **not met**. With strict comparison enabled, the test exits **134**: initial episodes cost 175 rectangles / 2.359 screens of fill, and 4,630 of 5,120 checked navigation frames exceed cast rectangles or fill. This remains a rendering acceptance failure; the test no longer approves it.

Navigation checks start at the first frame after each column change, cover 128 frames per move, and traverse all ten cards forward and backward with both ring and zoom focus. Forward moves allow at most two thumbnail uploads and 16 rasterizations for newly visible content; warm return moves allow zero. The observed first ring traversal uses one upload and four or five rasterizations when a new card enters; warm navigation and zoom navigation have zero. No moves exceed these bounds. The zoom setting change receives one warm-up before navigation to load the new maximum thumbnail size. Scrolled ring and zoom PNGs are generated in `/tmp`; no full-diff visual comparison was performed.

Follow-up experiments merged opaque thumbnail veil/watched tint and repeated faux-bold text submissions. The combined candidate still failed: 154 rectangles / 1.719 fill versus cast 169 / 1.610, 4,130 navigation frames over budget, churn=0 (rc 134). Pixel comparison also differed. Both experiments were reverted; these numbers are not final performance results. There is no FAIL-before/PASS-after proof, and neither P1 nor P2 is resolved.

TCL draw time <=25 ms still needs on-device measurement. Other screens, glass off, accent variants, and Tizen/webOS/Android/C9 performance remain unverified. No device deployment was performed.

The reused fixture lacks rating brand PNG paths (`/marcas/trakt.png`, `/marcas/imdb.png`); both images share that limitation. This is a hidden-window GL test, not an SDL dummy-video test: it requires an available OpenGL session and the existing Homebrew SDL libraries.
