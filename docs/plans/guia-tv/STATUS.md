# Guia de TV — estado do MVP (M1–M9)

Branch `feat/guia-tv` (worktree `/Volumes/ExternalSSD/nv-guia-tv`), com `agente/202-lgcanais` (b68bad96) mesclado.
Plano: `guia-tv-plano.md` (decisões do dono na seção 8).

| # | Item | Estado |
|---|---|---|
| M1 | Modelo por fonte, arena, cache por fonte, ids `fonte:id` | em andamento — `src/ltv.c` (arena, chaves, prefs, favoritos) |
| M2 | Sem corte silencioso, teto por plataforma, escolha de grupos | em andamento — `ltv_teto_canais`, `ltv_grupos_admitir` |
| M3 | Barra de fontes com saúde, "Todas" com selo | a fazer |
| M4 | XMLTV em fluxo + cache binário por fonte | a fazer |
| M5 | Casamento em cascata + manual + cobertura | a fazer |
| M6 | Fuso por fonte + linha `[ltv]` | a fazer |
| M7 | Teclas: Baixo/Azul, "0" último canal só na Live TV, botão | a fazer |
| M8 | Diagnóstico escolhe canais, grava por fonte; Android BEHIND_LIVE_WINDOW | a fazer |
| M9 | M3U e XMLTV por URL | em andamento — parser M3U em fluxo (`ltv_m3u_*`) |

## Testes
- `tests/ltv.sh` — arena, teto, grupos, numeração (decisão 7), dígitos, favoritos, prefs, M3U em fluxo.

## Não verificado
- Nada em TV (sem instalação, pedido do dono).
