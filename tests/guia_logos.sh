#!/bin/bash
# Real offline player_api/M3U/XMLTV parsers and guide rows; red/green PNG proof.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-guia-logos.XXXXXX")
trap 'rm -rf "$work"' EXIT
out=/Volumes/ExternalSSD/tmp/rev/livelogos
mkdir -p "$out"
flags=(-ffunction-sections -fdata-sections -Wl,-dead_strip)
sources=(src/ltv.c src/js.c src/idcanal.c src/dados.c src/marco.c src/guiaaddons.c
  src/xtream.c src/epg.c src/xmltv.c src/lembrete.c src/guialembrete.c
  src/fontecache.c src/cachearte.c src/cachedisco.c
  src/gfx.c src/text.c src/dobra.c src/bidi.c src/layout.c
  src/corviva.c src/gpunivel.c src/esmaecer.c)
compile() {
  cc "${flags[@]}" "$@" tests/guia_logos.c "${sources[@]}" \
    -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib \
    -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -O1 -g \
    -Wno-deprecated-declarations -Wno-macro-redefined -o "$work/test"
}
# Fixed parent preserves a reproducible pre-fix reference after commits.
git show 10d1d2961:src/guia.c > "$work/guia.c"
compile -DGUIA_LOGOS_BASELINE "-DGUIA_LOGOS_SOURCE=\"$work/guia.c\""
set +e
NUVIO_DADOS="$work/before" "$work/test" "$work/before" "$out/before.png"
rc=$?
set -e
if [ "$rc" != 1 ]; then echo "Expected baseline assertion failure (rc=1), got $rc"; exit 1; fi
compile
NUVIO_DADOS="$work/after" "$work/test" "$work/after" "$out/after.png"
echo "PASS: baseline rc=1; fixed rc=0; guide grid PNGs: $out"
