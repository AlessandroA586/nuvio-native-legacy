#!/bin/bash
# Offline regressions and PNGs; reuses the detail harness and its object cache.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR="/Volumes/ExternalSSD/tmp/jobtmp"
mkdir -p "$TMPDIR"
OUT=/Volumes/ExternalSSD/tmp/traktcoment
mkdir -p "$OUT"
mode=${1:-check}
if [ "$mode" = data ]; then
  cc -O1 -g -Wall -Wextra -DNV_EXTRAS_TESTE -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
    src/extras.c src/js.c tests/traktcoment_data.c -lpthread -o "$TMPDIR/traktcoment-data"
  "$TMPDIR/traktcoment-data"
  exit
fi
export NUVIO_SHOT_VIDRO=1 NUVIO_SHOT_VIDRO_FOSCO=1
if [ "$mode" = series ] || [ "$mode" = perf ]; then
  export NUVIO_TRAKT_CHECK="$mode"
  bash tests/detalhe_glass_shot.sh "$OUT" trakt-check
elif [ "$mode" = before ] || [ "$mode" = after ]; then
  for glass in 0 1; do
    NUVIO_SHOT_VIDRO=$glass bash tests/detalhe_glass_shot.sh "$OUT/$mode-glass-$glass" \
      filme-comentarios filme-comentarios-foco serie-comentarios serie-comentarios-pilula
    for bmp in "$OUT/$mode-glass-$glass/"*.bmp; do
      sips -s format png "$bmp" --out "${bmp%.bmp}.png" >/dev/null
    done
  done
else
  bash tests/traktcoment.sh data
  bash tests/traktcoment.sh series
  bash tests/traktcoment.sh perf
fi
