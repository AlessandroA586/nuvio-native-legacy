# Botão de microfone nas buscas (Samsung .tpk)

## O que é

Nas duas buscas — a da barra lateral e o Spotlight do CH- — o teclado do Nuvio Legacy continua
sendo o teclado de digitar. Ao lado do botão "Digitar pelo celular" fica um botão de microfone.
OK nele abre o **teclado da Samsung só para usar o microfone dele**:

1. OK no microfone → o teclado da Samsung sobe; o campo diz "Aperte o microfone do controle e
   fale." (não "Ouvindo…": o teclado ainda não ouve).
2. A pessoa aperta o microfone do controle e fala.
3. O texto entra inteiro no campo do app. Concluído busca; Voltar cancela; o aviso some.

## Por que o teclado da Samsung

O NuvioTVSmart e o Stremio `.wgt` usam o teclado da Samsung como teclado principal, e o
microfone do controle dita nele. A Samsung não dá a terceiros uma API de busca por voz com
texto livre (a VoiceInteraction entrega comandos e "Selecionar {título}", e só no `.wgt`), nem
uma chamada para o teclado já abrir ouvindo.

## Quando o botão aparece

Ajuste **Tela inicial › Mais opções › Barra lateral › Botão de microfone na busca** (só no
`.tpk`, só desta TV):

| Valor | Comportamento |
|---|---|
| **Automático** (padrão) | Aparece, menos na TV que diz não ter microfone (`http://tizen.org/feature/microphone` = false). Sem resposta da TV, aparece. |
| Ligado | Sempre aparece (TV detectada errado). |
| Desligado | Nunca aparece. |

Sem o campo do teclado da Samsung (o host não conseguiu montar o IME), nunca há botão.

Nos Ajustes a linha segue o padrão das outras: ícone de microfone, a frase de benefício ("Busque
falando, pelo microfone do controle."), a ajuda com os três valores, o "O que muda" (para
digitar continua o teclado do Nuvio) e uma cena gerada própria (`ajcHMicBusca`): a barra da
busca com o microfone no acento ao lado do celular, e o laço teclado da Samsung → controle
ouvindo → texto chegando. No Automático o controle ganha o selo de "a TV tem microfone"; no
Desligado fica a vaga vazia e o controle apagado. Os valores ficam no seletor da própria tela.

## Peças

- `tizen-tpk/Voz.cs` (build de release; o canário `NvTextoCanario=1` usa `Texto.cs`): um
  `TextField` 2x2 invisível, focável só enquanto o botão o usa, e o IME da Samsung sobre ele.
  Registra o botão com o que a TV respondeu sobre microfone. Nenhuma permissão nova: quem ouve
  é o teclado da Samsung.
- `src/entrada_texto.c`: `nv_tpk_voz_registrar` (host → C) e `texto_sistema_mic_modo` (o
  ajuste). Anuncia `TS_VOZ` sem `TS_TECLADO`, então digitar continua no teclado do app.
  `texto_sistema_voz_pelo_teclado()` diz que a voz é esse botão.
- `src/sistexto.c`: com o teclado da Samsung aberto pelo botão, o estado é `ST_DIGITANDO` e
  `st_aviso()` é o aviso do controle, limpo quando o teclado fecha.
- `src/teclado.c`: o teclado modal (chaves, nomes) não mostra "Falar" para esse caso.
- `src/main.c`: repassa o ajuste a cada quadro (como "Zoom no trailer").
- Ajustes: `src/ajustes.c` (opção, chave local `micBuscaLocal`, ajuda, benefício, "O que muda"),
  `ajustes_ux_tela.inc` (só no `.tpk`), `ajustes_ux_cenas_home.inc` (a cena),
  `ajustes_ux_visual.inc` (ícone), `ajustes_ux_dados.inc` (palavras da busca dos Ajustes),
  `ajustes_ux_secoes.inc` (a cena de referência marca a linha Buscar da barra lateral).
- Busca e Spotlight não mudaram: o botão já existia ao lado do celular, preso a
  `st_voz_disponivel()`.

## Outras plataformas

Android segue com o próprio ditado; LG não anuncia voz (sem botão); `.wgt` e `NuvioTpk40` ficam
como estavam. O ajuste não aparece fora do `.tpk`.

## Testes

- `tests/tpk_voz.sh`: a ponte com um host fingido (ajuste × microfone, aviso, Concluído,
  Voltar, fechar a tela).
- `tests/ajustes_ux_dados.sh`, `tests/ajustes_secoes.sh`: o ajuste (3 valores, padrão
  Automático, local) e a tela.
- `tools/idiomas.py`, `tools/varredura-i18n.py`: as 5 strings novas nas 29 tabelas (aviso do
  controle, nome do ajuste, ajuda, benefício, "O que muda").

## Falta na TV

Se o teclado da Samsung sobe sobre o app (mais provável no 8/9, que desenha na janela
principal) e se o microfone do controle dita nele. O log diz cada passo: `[texto] host voz …`.
