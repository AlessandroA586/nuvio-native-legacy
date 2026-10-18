#!/bin/bash
# One native geometry regression and optional before/after PNGs; no network.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=/Volumes/ExternalSSD/tmp/jobtmp
export NUVIO_DADOS
NUVIO_DADOS=$(mktemp -d "$TMPDIR/nuvio-heroisA.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/home.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/heroisA.c -Isrc -o "$NUVIO_DADOS/test" -O1 -g \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations \
  >"$NUVIO_DADOS/build.log" 2>&1 || { tail -45 "$NUVIO_DADOS/build.log"; exit 1; }
# Freeze the integration ancestor that preceded Hero A; never use a synthetic renderer.
base=0d464c26252ff9e0afef07fe55322da21ce6ce07
git show "$base:src/home.c" > "$NUVIO_DADOS/home.c"
git show "$base:src/layout.h" > "$NUVIO_DADOS/layout.h"
cc "${sources[@]}" tests/heroisA.c -Isrc -o "$NUVIO_DADOS/baseline" -O1 -g \
  -DHERO_BASELINE -DHOME_SOURCE=\""$NUVIO_DADOS/home.c"\" -include "$NUVIO_DADOS/layout.h" \
  -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wno-deprecated-declarations \
  >"$NUVIO_DADOS/base-build.log" 2>&1 || { tail -45 "$NUVIO_DADOS/base-build.log"; exit 1; }
NV_HEROIS_BASELINE=1 "$NUVIO_DADOS/baseline" > "$NUVIO_DADOS/base.log" 2>&1
set +e
"$NUVIO_DADOS/test" --transitions
transition_rc=$?
"$NUVIO_DADOS/test" --fade
fade_rc=$?
"$NUVIO_DADOS/test" "$@" > "$NUVIO_DADOS/current.log" 2>&1
layout_rc=$?
cat "$NUVIO_DADOS/current.log"
python3 - "$NUVIO_DADOS/base.log" "$NUVIO_DADOS/current.log" <<'PY'
import sys

def metrics(path):
    return {tuple(p[1:3]): list(map(float, p[3:]))
            for line in open(path) if (p := line.split()) and p[0] == 'BUDGET'}
base, current = map(metrics, sys.argv[1:])
assert len(base) == len(current) == 4, (base, current)
for state, values in current.items():
    print('GPU current/integration', state, values, base[state])
    # Draw count is diagnostic; the GPU budget is rasterized fill.
    assert values[2] <= base[state][2] + 0.000001, state
PY
budget_rc=$?
printf 'transitions rc=%d; fade rc=%d; layout rc=%d; budget rc=%d\n' "$transition_rc" "$fade_rc" "$layout_rc" "$budget_rc"
[ "$transition_rc" -eq 0 ] && [ "$fade_rc" -eq 0 ] && [ "$layout_rc" -eq 0 ] && [ "$budget_rc" -eq 0 ]
