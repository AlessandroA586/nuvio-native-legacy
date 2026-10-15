# Vidro todo - inventory (branch agente/205-vidrotodo)

Owner: "varios componentes nao estao em glass, estao diferentes um do outro e nem acompanhando o ajuste de opacidade".

Standard after this branch:
- Panel role (dialogs, sheets, menus, dropdowns, toasts, islands, sidebar, rail, clock island): `gfx_vidro_material` - one tint (gray .06), density x "Opacidade do vidro", shared backdrop blur.
- Control role (rest pills/chips/buttons/inner tiles): `gfx_vidro_painel` - same blur path, gray .16 (one step lighter so it reads on a panel), fundo x opacity.
- No-blur / old TV ("Vidro fosco" off, efeitos minimos, over video, nested): `gfx_vidro` draws the same scrim flat, same density x opacity, for every surface (uniform, like the clock island).
- Glass off: per-caller solid colour (`gfx_vidro_ou_solido`, explicit else).
- Focus: unchanged (`gfx_vidro_pilula_cheia` / `gfx_vidro_foco` / `gfx_vidro_cartao`).

Proof: `bash tests/vidro_todo.sh` (FAIL before: accent pill and player buttons ignored opacity, sheet tone 46 vs 37.8; PASS after).

## Changed in this branch

| file:line (before) | surface | before | after | follows opacity before -> after |
|---|---|---|---|---|
| gfx.c gfx_vidro_folha | all sheets (menu, Fontes, Salvos, guia, agenda, amigos, spotlight, ctxmenu, streams) | gfx_vidro gray .09 at opacity | gfx_vidro_material(DENS) | y -> y (tone unified) |
| gfx.c gfx_vidro_painel_acento | Play / selected pills (botoes, detail) | flat gfx_cor .16 + accent | gfx_vidro_painel + accent | n -> y |
| ilha.c:2223 | clock / notification island | gfx_vidro_painel gray .16 | gfx_vidro_material(fundo) | y -> y (tone unified) |
| menu.c:340 | rail | gfx_vidro gray 15/255 | gfx_vidro_material | y -> y |
| menu.c:1209 | Apple TV sidebar | gfx_vidro .09 + flat miolo floored at .88 (slider ignored at <= 88 %) | gfx_vidro_material(TV_MIOLO_MIN) | n -> y |
| recenviar.c:661 | send-to-friend island | own shadow + folha + light | plrui_material(modal) | y -> y |
| homevazio.c:44 | empty-home card | gfx_vidro_painel gray .16 | gfx_vidro_material(DENS) | y -> y |
| vertudo.c:1848 | collection category dropdown | gfx_vidro_painel gray .16 | gfx_vidro_material(DENS) | y -> y |
| plrui.c:47 plrui_botao_repouso | player/overlay rest buttons (12 callers) | white .08 veil | gfx_vidro_painel(.55) | n -> y |
| estatvivoui.c:419 | stats panel tile | opaque gfx_cor .118 | gfx_vidro_painel(.72) / solid with Glass off | n -> y |
| amigoperfil.c:189 | friend profile stat card | opaque gfx_cor .118 | gfx_vidro_painel(.72) / solid with Glass off | n -> y |

## Not touched (out of scope / exceptions)

| file:line | surface | material | follows opacity | why |
|---|---|---|---|---|
| menu.c:339 | top bar | gfx_vidro BARRA (k inside) | y | agente/205-barra27 |
| home.c Cover Flow strip | - | - | - | agente/205-cfrestaura |
| vertudo.c collection header | - | - | - | agente/205-colhdrlinha |
| estatvivoui.c internals other than tile | - | - | - | agente/205-estatdicas |
| gfx_vidro_superficie (2) | inner veil on a sheet | white .05 | n (sits on glass that does) | inner row, not a surface |
| plrui_linha_foco, focus helpers | focus | white | n | focus stays as is |
| detail.c:5369, 5724, 6026 | Apple-style focus info backing | gray .19 .6 x focus | n | focus backing, not rest surface |
| home.c:1520, badges.c:329, streams.c:3462, explorar.c:1070, guia.c:3040 | rating / source / channel chips | gfx_cor | n | small chips over art (vidro-padrao exceptions) |
| episodios.c:847, posplay.c:670 | per-row chip | gfx_cor vidro-ok | n | glass pass per row blows budget |
| biblioteca.c:2337, home.c:7557, salvospainel.c:3030 | progress tracks | gfx_cor | n | progress bar, not surface |
| edge light | plrui_material/recenviar: corner light; ajIlhaMaterial: none; island: own top glow | - | - | pending: not unified (layout/look risk) |
| docs/issues/vidro-padrao.md list | rest of vidro_padrao exceptions | - | - | unchanged |
