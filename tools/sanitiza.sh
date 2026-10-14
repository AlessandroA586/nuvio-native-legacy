#!/bin/bash
# Runner OBRIGATORIO de sanitizadores (fase 0 do docs/plans/rust-mapa.md).
# Roda a suite existente de tests/ com ASan+UBSan e, SEPARADAMENTE, os 28
# testes de fio com ThreadSanitizer. Nunca os dois no mesmo build: TSan e
# address sao mutuamente exclusivos e o compilador aborta a mistura.
#
#   bash tools/sanitiza.sh          # asan + thread
#   bash tools/sanitiza.sh asan     # so ASan+UBSan
#   bash tools/sanitiza.sh thread   # so TSan
#
# Regra de gate: rc != 0 (erro de sanitizador reproduzivel) BLOQUEIA.
# SKIP / dependencia ausente e reported separado e NAO conta como passou.
cd "$(dirname "$0")/.." || exit 1
LOGS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-sanitiza.XXXXXX") || exit 1

# Os 28 scripts com -fsanitize=thread fora de comentario. Lista estatica de
# proposito: e o contrato do job TSan e nao deve depender de grep no proprio
# runner (o runner e a unica fonte de "quem roda com TSan").
TSAN_TESTS="audsync autosync cacheboost capmkv capmkv_url_longa cateps_corrida
colhash_corrida credfio desc_iniciar_corrida emby fontecache jellyfin legenda2
legref legsync p2pmotor p2pmotor_real plex pluginjs plugins_fontes rede_pedido
sessao_corrida streamfit streamfit_diagnostico streamfit_passiva
sync_resumo_corrida trakt_play_corrida vistoep_corrida"

# Mesmas exclusoes de tools/testa-tudo.sh: precisam de arte/GL, levantam
# servidor que nunca sai, ou medem quadro por quadro. Nao sao teste de logica.
foraDaSuite() {
  case "$1" in
    *_shot.sh|cinematic.sh|director.sh|webp-tizen.sh|tizen-clock.sh|salvospainel_perf.sh|fluidez_perf_player.sh|fluidez_perf_guia.sh) return 0 ;;
  esac
  return 1
}

# Regras de gate: rc!=0 por ERRO DE SANITIZADOR (reproduzivel) ou por teste
# que quebra sozinho BLOQUEIA. SKIP e DEPENDENCIA (motor/SDK ausente ou
# pre-compilado incompativel) sao reportados a parte e NAO contam como passou.
passou=0; bloqueou=0; semProva=0; fora=0; dep=""
mkdir -p "$LOGS/asan" "$LOGS/thread"
rodar() {
  local nome="$1" modo="$2" log="$LOGS/$2/$1.log"
  local san=1; [ "$modo" = thread ] && san=thread
  mkdir -p "$LOGS/$modo"
  if SANITIZE="$san" ASAN_OPTIONS="abort_on_error=1" \
     UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
     TSAN_OPTIONS="halt_on_error=1:exitcode=66" \
     bash "tests/$nome" >"$log" 2>&1; then
    if grep -Eq '(^|[^A-Za-z])SKIP([^A-Za-z]|$)' "$log"; then
      echo "SKIP  $nome ($modo: $(grep -Em1 'SKIP' "$log" | cut -c1-80))"
      semProva=$((semProva + 1)); return 0
    fi
    grep -Eq 'PULADO|pulados|sem resultado' "$log" && echo "ok*   $nome ($modo: casos pulados no log)"
    echo "ok    $nome ($modo)"
    passou=$((passou + 1)); return 0
  fi
  local sanEro='ERROR: (Address|Leak)Sanitizer|SUMMARY: .*Sanitizer|runtime error:|WARNING: ThreadSanitizer'
  # Falha de compilacao/link = dependencia ausente ou pre-compiladovelho:
  # o script nem chegou a rodar instrumentado.
  if grep -Eq '(^|:)[0-9]+:[0-9]+: error:|No such file or directory|ld: |fatal error:' "$log" \
     && ! grep -Eq "$sanEro" "$log"; then
    echo "DEP   $nome ($modo: $(grep -Em1 'error:|No such file' "$log" | cut -c1-80))"
    dep="$dep $nome"; semProva=$((semProva + 1)); return 0
  fi
  if grep -Eq "$sanEro" "$log"; then
    echo "ERRO DE SANITIZADOR  $nome ($modo)"
    grep -E "$sanEro" "$log" | head -4
  else
    echo "FALHA $nome ($modo)"
  fi
  tail -8 "$log"
  bloqueou=$((bloqueou + 1))
}

modo="${1:-todos}"
case "$modo" in asan|thread|todos) ;; *) echo "uso: $0 [asan|thread|todos]"; exit 2 ;; esac

if [ "$modo" != thread ]; then
  echo "== ASan+UBSan =="
  for f in tests/*.sh; do
    n=$(basename "$f")
    foraDaSuite "$n" && { fora=$((fora + 1)); continue; }
    # sync_resumo_corrida e trakt_play_corrida tem TSan fixo no codigo: nao
    # aceitam ASan. Rodam no passo thread.
    case "$n" in sync_resumo_corrida.sh|trakt_play_corrida.sh) continue ;; esac
    rodar "$n" asan
  done
  echo "asan: $passou passaram, $bloqueou bloquearam, $semProva sem prova; fora da suite: $fora"
fi

if [ "$modo" != asan ]; then
  p=$passou; b=$bloqueou; sp=$semProva
  echo "== ThreadSanitizer (28 testes de fio) =="
  for t in $TSAN_TESTS; do rodar "$t.sh" thread; done
  echo "thread: $((passou - p)) passaram, $((bloqueou - b)) bloquearam, $((semProva - sp)) sem prova"
fi

echo
echo "TOTAL: $passou passaram, $bloqueou bloquearam, $semProva sem prova (SKIP+dependencia); logs: $LOGS"
[ -n "$dep" ] && echo "sem dependencia: $dep"
[ "$bloqueou" -eq 0 ]