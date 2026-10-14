# Teclas CH+ / CH− no lugar das cores (2.0.5)

Pedido do dono: "padronizar onde fala para apertar o botão azul trocar para o
CH+ e o outro botão para o CH−, em todas as plataformas".

## Quem é "o outro botão"

**AMARELO.** Azul abre Salvos, amarelo abre a busca (Spotlight). O Android e o
.tpk já ligavam CH+ → AZUL/Salvos e CH− → F5/Spotlight, então o par é
CH+ = azul e CH− = amarelo. Vermelho (log) e verde (diagnóstico da Live TV)
não são "o outro": seguem como estão (lista no fim).

## Comportamento das teclas (todas as plataformas)

| Contexto | CH+ | CH− | Cor |
|---|---|---|---|
| Fora de zap (Home, telas, Ajustes…) | Salvos / cartão da ilha (= AZUL) | Spotlight, ou fecha o Spotlight (= AMARELO) | AZUL e AMARELO continuam iguais |
| Guia de TV, player e PiP com canal no ar | zap de canal (inalterado) | zap de canal (inalterado) | AZUL/AMARELO inalterados; dica de cor mantida |
| Segurar CH+ | abre a Central de controle (`src/chsegura.h`, `src/central.h`) | — | toque curto continua fazendo a ação de AZUL |
| Detalhe de fonte (`streams.c`) | — | abre, como AMARELO | AMARELO inalterado |
| Guia de TV, CH− | categoria seguinte (já existia; o `spotAtalho` recusa no guia) | | |

### Ligações acrescentadas

- **webOS (Emscripten), `src/main.c`**: `remapCanal` agora olha `app_zap_ativo()`.
  Com canal na tela, F7/F8 viram CH_UP/CH_DOWN (zap de verdade); fora disso
  CH+ → "s" (Salvos) e **CH− → F5 (Spotlight)**. Antes o CH− não fazia nada fora do zap.
- **`src/app.c` (`spotAtalho`), `src/spotlight.c`, `src/streams.c`**: CH_DOWN
  abre/fecha o Spotlight e abre o detalhe de fonte, como AMARELO.
- **Tizen .wgt, `tools/tizen-shell.html`**: ChannelDown (428) registrada e
  traduzida para F8 (keydown/keyup), par do F7 do CH+.
- **Android TV/mobile e .tpk**: já tinham CH+/CH−; só os textos mudaram.
- Teste: `tests/wgt_voz_shell.js` (via `bash tests/wgt_voz.sh`) prova que
  428 → F8 (119) em keydown e keyup, contexto que antes não tinha a ligação.

## Inventário de textos

| Texto (pt) | Tela | Tecla antes | Tecla agora |
|---|---|---|---|
| "AMARELO abre a busca… AZUL ou CH+ abre Salvos…" | Ajustes › guia de teclas | AMARELO / AZUL ou CH+ | CH− / CH+ |
| "A tecla AMARELA abre o Spotlight…" | Ajustes › guia (Spotlight) | AMARELA | CH− |
| "AMARELO no controle, ou Busca no menu" | Ajustes › guia (Spotlight) | AMARELO | CH− |
| "A tecla amarela abre a busca…" | Novidades 1.7 | amarela | CH− |
| "AMARELO · livre" + corpo | Novidades 1.1 | AMARELO (texto "não faz nada") | trocado por "CH− · a busca" |
| "CH- · o registro" (Android) | Novidades 1.1 | CH- (hífen ASCII) | "Info · o registro" |
| "AZUL · Salvos e o guia" / "CH+ · Salvos e o guia" | Novidades 1.1 | AZUL (LG) / CH+ (Android) | um só: "CH+ · Salvos e o guia" |
| "…pílula… AZUL ou CH+ abre, → leva aos Salvos." (2x) | Ajustes › guia; Novidades 1.7 | AZUL ou CH+ | CH+ |
| "AZUL ou CH+ na Home" | Ajustes › guia (ilha) | AZUL ou CH+ | CH+ |
| "AZUL abre o painel com o que você guardou…" | Ajustes › guia (Salvos) | AZUL | CH+ |
| "AZUL › Avisos", "AZUL › Amigos" | Ajustes › guia | AZUL | CH+ (sem chave na tabela; só fonte) |
| "…CH+ ou AZUL retoma de onde parou." | Ajustes › guia (sair do player) | CH+ ou AZUL | CH+ |
| "…(CH+ ou AZUL)" `AJ_SAIDA_PLAYER` | Ajustes › player | CH+ ou AZUL | CH+ |
| Pílula `AZUL` (`TXT_AJ_KBD`) | Ajustes › guia (tecla) | AZUL | CH+ |
| "Painel Social abre com o Azul" | Novidades 2.0 | Azul | CH+ |
| Pílulas `Azul` (2x) | Novidades 2.0 | Azul | CH+ |
| "…um toast quando chega, AZUL abre…" | Novidades 1.3.2 | AZUL (LG) / CH+ (Android) | um só: CH+ |
| "Está na tecla AZUL" | Recomendações (intro) | AZUL (LG) / CH+ | "Está no CH+" |
| "Depois, o botão AZUL / CANAL + abre sua lista…" | Intro de Salvos | AZUL / CANAL + | "Depois, o CH+ abre sua lista…" |
| Desenho da tecla | Intro de Salvos | pastilha azul (LG), rocker (Samsung), pastilha CH+ (Android) | pastilha "CH+" em todas |

Traduções: pt e as 28 `idioma_*.h` atualizadas juntas (chaves renomeadas,
duas novas — "CH− · a busca", "Info · o registro" — e 9 obsoletas apagadas).
No CH− usa-se o sinal de menos U+2212, como na chave já existente
"CH+ e CH− zapeiam no canto".

## Contextos mantidos (CH+/CH− já têm outro significado)

Aqui a dica de cor continua, porque CH+/CH− fazem zap e a cor é a única
tecla que faz a ação:

- `src/guia.c:4191`: rodapé do guia de canais ("Azul · Guia completo"; no Android já era "CH+").
- `src/guia.c:4918-4919`: "…Voltar ou Azul fecha".
- `src/player.c:2461`: dica do PiP ("Azul · Tela cheia"; no Android já era "CH+").
- `src/pipintro.c:253`: "Azul volta na hora".
- `src/novidades.c:204` e `src/ajustes_ux_guia_dados.inc:247`: "BAIXO ou AZUL abrem o mini guia" (CH+/CH− zapeiam ali).

## Para o dono decidir (vermelho e verde, mantidos)

- Vermelho: "VERMELHO · o registro" (Novidades 1.1, só LG/Samsung; Android diz "Info").
- Verde: "VERDE · só no Samsung" (Novidades 1.1); "Verde: diagnóstico da Live TV" (`guia.c:4936`);
  "VERDE testa os canais" (guia de teclas e `ajustes_ux_guia_dados.inc:265,325`).
- Amarelo no guia de TV: "AMARELO troca grade e cartões" (`ajustes_ux_guia_dados.inc:25,244`). Aqui
  o CH− é salto de categoria, então a cor continua sendo a única tecla.
- Cores de legenda/tema (`ajustes.c`, `video.c`): não são teclas.

## Central de controle

Segurar CH+ abre a Central de controle; o toque curto faz a ação de CH+/AZUL
(Salvos ou cartão da ilha). Está dito no guia de teclas ("Segurar CH+ abre a
Central de controle."); a lógica é a de `src/chsegura.h` / `src/central.h` e
não mudou.
