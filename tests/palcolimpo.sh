#!/bin/bash
# Real GL rendering: three Stage modes, focus fades, layouts, scale and glass.
set -eu
cd "$(dirname "$0")/.."
output="${1:-/Volumes/ExternalSSD/tmp/rev/palcolimpo}"
phase="${2:-after}"
mkdir -p "$output"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-homelayouts-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/home.c) continue;; esac
  sources+=("$source")
done
baseline_flags=()
if [ "$phase" = before ]; then
  git show f246d8e919a4763ce467f5d3ecbaa363f20e8512:src/home.c > "$NUVIO_DADOS/home-baseline.c"
  baseline_flags+=("-DPALCO_BASELINE=\"$NUVIO_DADOS/home-baseline.c\"")
fi
cc "${sources[@]}" "${baseline_flags[@]}" tests/palcolimpo.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -g -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wno-deprecated-declarations >"$output/$phase-build.log" 2>&1
rc=0
for scale in 0.85 1; do
  for glass in 0 1; do
    case_data=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-homelayouts-shot.XXXXXX")
    NUVIO_DADOS="$case_data" "$NUVIO_DADOS/shot" "$output" "$phase" "$scale" "$glass" \
      >"$output/$phase-$scale-g$glass.log" 2>&1 || rc=$?
    rm -rf "$case_data"
    tail -n 6 "$output/$phase-$scale-g$glass.log"
  done
done
for bmp in "$output/$phase-"*.bmp; do
  [ -e "$bmp" ] || continue
  sips -s format png "$bmp" --out "${bmp%.bmp}.png" >/dev/null
  rm "$bmp"
done
if [ "$rc" = 0 ] && [ "$phase" = after ]; then
  python3 tests/palcolimpo_pixels.py "$output" || rc=$?
fi
exit "$rc"
