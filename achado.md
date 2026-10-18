# tpk60preta — finding

## Evidence and diagnosis

The log contains `ok video-window`; it does not support the original video-window failure diagnosis. It records roughly 12.3 seconds between `gfx_iniciar` and font initialization on the affected Mali TV. That interval covers all shader initialization; the glass shader's individual contribution was not measured. No startup speedup is established.

## Final implementation

- `src/gfx.c`: build all shader programs in `gfx_iniciar`, including glass. Remove the frame-3 lazy link from `gfx_vidro`. Neither `gfx_rect` nor `gfx_vidro` compiles or links a shader during interactive rendering. This deliberately gives up the proposed startup optimization to avoid moving an unbounded cold link stall into user interaction on Tizen, webOS/C9, and Android.
- A rejected glass link leaves its program unset and draws the existing flat panel and border. Drawing never retries compilation. Other program link failures still fail initialization.
- `tests/gfx_vidro_preguica.c`: require the program count to remain unchanged across the first glass draw, a draw after several frames, and reuse. Retain darkened glass pixels, the subsequent red rectangle, and GL error checks. Optional `NV_GLASS_SHOT_PREFIX` saves each framebuffer as PNG.
- `tizen-tpk/NuvioTpk40/Program40.cs`: unchanged. The proposed catch-and-continue video-window fallback was reverted because the logged window creation succeeded and the host change was unrelated to the demonstrated shader stall. There is no new failed-window recovery protection.

## Regression proof

The revised target test against the previous implementation returned 1: the late glass draw increased the program count from 46 to 47. The final implementation must keep the count at 47 throughout drawing. Before/after PNGs are saved under `/Volumes/ExternalSSD/tmp/rev/tpk60preta/`.

## Limits

No device installation, deployment, startup timing measurement, approved-layout comparison, forced-link-failure injection, or glass-off/accent matrix was performed. Native GL coverage does not establish TV driver timing. A future startup optimization needs measured timings and a mechanism that does not compile/link synchronously during interaction.
