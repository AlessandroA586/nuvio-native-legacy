# Modern episode row rendering

Branch: `agente/205-episodioslento`.

The strict regression is unchanged: `bash tests/detail_episode_perf.sh` compares every episode navigation frame against cast rectangles and GPU fill. Ten episodes, glass enabled, forward/back traversal, ring and zoom focus, 128 frames per move.

FAIL before: rc 134, 175 rectangles / 2.359 screens against cast 169 / 1.610; 4,630 of 5,120 navigation frames exceed budget. Log: `/tmp/nv205-fail-before.log`.

PASS after: rc 0, 118 rectangles / 1.429 screens against cast 169 / 1.610; all 5,120 frames within budget, churn=0. Log: `/tmp/nv205-final-test.log`.

The renderer combines episode art, reveal fade, black veil and watched tint in one opaque pass. Translucent cards retain separate overlays. The backdrop skips solid thumbnail interiors, keeping rounded corners and their antialias fringe. Invisible submissions are culled while navigating Modern episodes. Text styles supply weight without repeated faux-bold draws.

Final build: `NUVIO_MAC_SO_COMPILAR=1 NUVIO_MAC_BIN=/tmp/nv205-final-mac bash tools/mac.sh`, rc 0. Log: `/tmp/nv205-final-build.log`.

Before/after PNGs: `/Volumes/ExternalSSD/tmp/rev/episodioslento/{before,after}-{episodes,ring-scroll,zoom-scroll}.png`. Initial and scrolled ring/zoom captures are retained; text weight changes intentionally, geometry remains unchanged. These are visual evidence, not pixel-identical output claims.

TCL <=25 ms and Tizen/webOS/Android/C9 device budgets remain unmeasured. Other screens, glass-off and accent variants have no new visual verification. No deployment performed.
