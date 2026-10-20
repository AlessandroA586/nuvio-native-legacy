# Nuvio Legacy 2.0.8

Tudo mergeado em `integracao/2.0.5` depois da tag `v2.0.7` (base `f840b4037`). Última sync: 2026-10-20T10:21:00-03:00.
Comunidade: **https://discord.gg/9NWr6SHyzJ**

> Card in-app (`src/novidades/208.inc`) será a lista curta do owner; este arquivo é a fonte completa.

---

## Destaques

(nenhum ainda)

## Novidades

(nenhuma ainda)

## Correções

- **Tizen 9/10 abre sem tela preta**: o app ignora a superfície EGL 1x1 que o sistema entrega no primeiro quadro e espera o tamanho real — deve resolver o "abre preto, fecha e reabre" reportado na 2.0.7. Samsung .tpk. Refs #624 #628 #629, `f840b4037`. (aguardando validação na TV)

## Desempenho

(nada ainda)

## Comunidade

(nada ainda)

## Interno

- `d2a392e14` docs(map): sync tracking — mapa de issues.

## Pendente (ainda não integrado)

Branches em revisão/OK para integrar (ver estado.md): agente/205-i429hbs (#429), agente/205-p2pusb (#334 #505), agente/205-i513autoplay (#513), agente/205-barra27, PRs #489 #490 #491 #493 #498, agente/205-dicatoque (#472), m3umisto, palco2, logos, fileiraui, diretorpol, triagem-velhas.

---

## Release features (English)

### Highlights

(none yet)

### New

(none yet)

### Fixes

- **No more black screen on Tizen 9/10**: the app ignores the 1x1 placeholder EGL surface the system reports on the first frame and waits for the real size — should fix the "opens black, reopen to fix" reports on 2.0.7. Samsung .tpk. Refs #624 #628 #629, `f840b4037`. (pending TV validation)

### Performance

(nothing yet)

### Community

(nothing yet)

### Internal

- `d2a392e14` docs(map): sync tracking — issue map housekeeping.

### Pending (not integrated yet)

Branches in review / OK to merge (see estado.md): agente/205-i429hbs (#429), agente/205-p2pusb (#334 #505), agente/205-i513autoplay (#513), agente/205-barra27, PRs #489 #490 #491 #493 #498, agente/205-dicatoque (#472), m3umisto, palco2, logos, fileiraui, diretorpol, triagem-velhas.
