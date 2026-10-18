# Modern episode row rendering

Branch: `agente/205-episodioslento`.

`bash tests/detail_episode_perf.sh` exercises actual GL rendering and episode navigation: ten episodes, glass enabled, forward/back traversal, ring and zoom focus, 128 frames per move.

Typography regression FAIL before: rc 134, `Typography mismatch: style=7 weight=0.50 alpha=1.00`. The shared helper discarded the approved faux-bold weight. Fresh log: `/Volumes/ExternalSSD/tmp/rev/episodioslento/fail-before.log`.

PASS after: rc 0, episodes 118 rectangles / 1.429 screens against cast 169 / 1.610; all 5,120 navigation frames within budget, churn=0. Log: `/Volumes/ExternalSSD/tmp/rev/episodioslento/pass-after.log`.

The helper now composes the original pixel-snapped glyph copies in one submission using the cached line texture. Each copy retains its original alpha, including fades and group opacity. Layout dimensions and font styles remain unchanged. The regression compares rendered pixels against the original repeated draws for season/tab/title styles, weights 0/0.5/1/1.1/1.4, integer/fractional positions and full/faded alpha. Maximum permitted channel difference is 2/255 because the original path rounds framebuffer blending after each copy.

Fresh before/after PNGs: `/Volumes/ExternalSSD/tmp/rev/episodioslento/{before,after}-{episodes,ring-scroll,zoom-scroll}.png`. The episode pair also replaces the stale committed captures in this directory. Before shows the reported thin regression; after restores approved weight. Hashes differ.

Tizen/webOS/Android/C9 timing remains unmeasured. Glass-off, accent variants and complete Standard layout screenshots remain unverified; the shared-helper pixel regression covers the typography operation used by those layouts. No device-budget conclusion, push or deployment.
