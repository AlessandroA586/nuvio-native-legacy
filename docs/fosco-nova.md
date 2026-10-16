# Vidro fosco: backdrop-only source (2.0.5)

Owner, 16/10: "o frost ta uma merda ainda, ou a gente pesquisa outra abordagem
ou vamos tirar".

## Why the old path costs

`gfx_vidro` -> `vidroFonte` -> `vidCapturar` (src/gfx.c). Each capture:

1. `glCopyTexSubImage2D` of the **whole screen** (1920x1080, 4K on some TVs)
   into `vidGrab`, mid-frame. On a tiler (Mali-G52/G71 in the TCL/C9, the
   webOS/Tizen SoCs) that forces the tile list drawn so far to resolve to
   memory before the copy, then the frame continues with a reload.
2. Five full-screen passes: 1/2 box, 1/4, two Kawase, back to 1/2.

Reuse across frames works only while `vidroHash` (every draw that reaches what
the glasses read) is unchanged. On Home the top bar, avatar and clock island
read the band where the rows scroll and the focus ring moves, so every
scrolling/focus frame recaptured: roughly 2.1 M px copied + ~1.3 M px of
blur fill per frame at 1080p (4x at 4K), plus the tile flush. The old
`perf(glass)` commits (d98d72026, 0c77460c1, 5536ee8da, d7ec89975) cut the
number of captures to one per frame, not that one.

It is also ugly: the glass blurs whatever passes under it (poster edges,
text, focus ring), so the bar shimmers while rows scroll. The green cast
(14/10) came from the old palette source, fixed by reading the screen
(tests/vidro_neutro.sh); nothing below reintroduces palette colour.

## Alternatives (cost per frame, Mali-class GPU, 1080p)

| | Moving content (scroll/focus) | Backdrop changes (hero swap, detail scroll) | Static | Look |
|-|-|-|-|-|
| old: capture all under glass | 1 copy + 5 passes | same | 0 | blurs posters/text, shimmers |
| (a) pre-blur the art once (`gfx_desfocado`, 96x54) | 0 | 0 (one 96x54 blur per new art) | 0 | needs per-screen art + rect mapping; loses the hero ramps, light and veils, so glass over the dark ramp shows bright art |
| (a') **backdrop-layer capture** (chosen) | 0 | 1 copy + 5 passes | 0 | exact composite of art + ramp + light + veils; no posters/text |
| (b) 1/8-res capture + dual Kawase on change | 1 small copy + 4 tiny passes | same | 0 | still blurs content; 1/8 shows blocky shimmer |
| (c) tinted translucent panel, no blur | 0 | 0 | 0 | gray, not frosted |

(a') is (a) done generically: the "pre-blurred backdrop" is the screen's own
backdrop layer, captured the frame it changes, so every screen (Home hero,
detail art, Settings Frost/Arte borrada, Explore sky, social background)
works with no per-screen code and no colour or ramp mismatch.

## What runs now (16/10, option (a))

Owner, 16/10: "antes na c9 o frozen era mais rapido que o glass" — until
c84afbf04 the frost never captured the screen. Back to that source:

- The frame's backdrop art (the first full-screen-ish art draw of the backdrop
  layer at alpha >= 0.5: hero, detail, vitrine, arte) is blurred ONCE into
  96x54 by `gfx_desfocado` (cached per texture); the "Arte borrada"/"Frost"
  bakes are blurred already and are used as is (`vidroCanal`).
- `GFX_VIDRO` samples it through `uMapa` (screen UV -> art UV, centred
  cover). No `glCopyTexSubImage2D`, no blur pass, on any frame
  (`gfx_vidro_capturas`/`gfx_vidro_passadas` stay 0).
- The green cast (14/10) came from the title palette light (320x180, +35%
  saturation) used as the frost source. The palette never enters the frost;
  only the art, blurred (tests/vidro_neutro.sh: green palette, gray art,
  neutral glass).
- No art behind (flat colour screens), video under the glass, "Vidro fosco"
  off, minimal effects -> the flat `chapado` scrim, as before.

Known differences vs the 15/10 screen capture (PNGs in
/Volumes/ExternalSSD/tmp/foscovelho/cmp): the hero ramps and veils drawn
after the art are not in the source, so glass over the dark left ramp (avatar
chip) and the consent modal over its veil read a little lighter; during a hero
crossfade the frost switches art at the midpoint.

## Measured (Mac, offline harnesses)

- tests/vidro_orcamento.sh (hero scroll, hero crossfade, detail scroll):
  HEAD 1 capture + 5 passes/frame on crossfade and detail; now 0 and 0.
- tests/vidro_c9.sh (Home, menu focus moving, backdrop changing): 0.35
  captures + 1.75 passes/frame -> 0 and 0, all tiers, 1080p and 4K.
- tests/vidro_telas.sh: Explore 0.04 -> 0 captures/frame; others 0.
