# Modern episode row rendering

Branch: `agente/205-episodioslento`.

The glass rating layout calls `desenhaCartao(..., alpha=0)` to measure its height. Episode section position calculations repeat that measurement. `txt_desenhar_tom` submitted those invisible text quads anyway. The shared text renderer now skips alpha-zero draws while preserving measurement and cached line dimensions.

Episode thumbnails request a stable maximum focus width, preventing successive 32-pixel decode promotions during zoom. Episode culling keeps only the focus rim/zoom margin; hidden cards do not reach texture requests or text layout. Existing text, image and glass caches remain in use; no additional cache is needed for the invisible draws.

## Reproduction and results

`bash tests/detail_episode_perf.sh` renders the real Modern detail screen in a hidden OpenGL window/FBO, without network fixtures. It uses ten episodes with local thumbnails and glass enabled, a deterministic detail animation clock, a complete navigation pass, a measurement-only draw check, and a 220-rectangle / 3.2-screen fill cap.

With the alpha guard absent: **rc 134**, 343 rectangles, 3.779 screens of fill; cap assertion fails. With the fix: **rc 0**, 175 rectangles, 2.359 screens of fill. Settled navigation frames have zero texture uploads and zero text rasterizations. Static glass requires zero captures during the measured eight frames.

`episode-before-episodes.png` and `episode-after-episodes.png`: ImageMagick AE comparison **0 changed pixels**. The comparison isolates the alpha guard; episode geometry changes do not affect this initial, ring-focused capture.

Final build: `NUVIO_MAC_SO_COMPILAR=1 NUVIO_MAC_BIN=/tmp/nv205-episode-mac bash tools/mac.sh`, **rc 0**. Existing `ATOMIC_VAR_INIT` deprecation warning.

## Limits

The cast-focused screen measures 169 rectangles / 1.610 screens of fill: exact cast parity is **not met**. The episode screen is under the regression cap and saves 49% of submitted rectangles and 38% of fill, but TCL draw time <=25 ms needs on-device measurement. No device deployment was performed.

The reused fixture lacks rating brand PNG paths (`/marcas/trakt.png`, `/marcas/imdb.png`); both images share that limitation. This is a hidden-window GL test, not an SDL dummy-video test: it requires an available OpenGL session and the existing Homebrew SDL libraries.
