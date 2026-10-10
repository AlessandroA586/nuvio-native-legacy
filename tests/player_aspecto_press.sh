#!/bin/bash
# The aspect key: which mode the press lands on, and what reached the video plane.
# See tests/player_aspecto_press.c. No TV, no GL, no network.
#
# Only the five geometry calls the plane is reached through are replaced: the
# build script renames the real ones so player.c links against the test's own
# video_janela/video_janela_fonte/video_largura/video_altura/video_recorte_fonte.
# Everything else comes from src/video.c, which answers state questions with no
# device present.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-aspecto-press.XXXXXX")
trap 'rm -rf "$work"' EXIT

cflags=(-O1 -g -Isrc $(sdl2-config --cflags) -Wno-deprecated-declarations)

# ONLY the four calls the plane is reached through, and nothing else: the rest
# of src/video.c keeps working so the player sees a real backend underneath.
#
# video_recorte_fonte is NOT renamed. It comes from src/video.c, which compiles
# WITHOUT NV_TPK here and answers 1 - the target that crops, i.e. the LG and the
# Android. The .tpk's answer (0, or 1 only with the ZOOM_ROI canary) is a
# compile-time question already covered by tests/tpk-roi.sh; what this test
# pins is what the PRESS does once the answer is 1.
renomes=()
for sym in video_janela video_janela_fonte video_largura video_altura video_tocar; do
  renomes+=("-D$sym=nv_base_$sym")
done
cc "${cflags[@]}" "${renomes[@]}" -c src/video.c -o "$work/video.o"

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/video.c) continue;; esac
  sources+=("$source")
done
# app.c calls the log sink's step. The module only exists on a build that ships
# it, so it is added here only when the file is present.
extra=()
if [ -f src/logenvio.c ]; then
  cc "${cflags[@]}" -DNV_LOG_URL='""' -c src/logenvio.c -o "$work/logenvio.o"
  extra+=("$work/logenvio.o")
fi

cc "${cflags[@]}" "${sources[@]}" tests/player_aspecto_press.c "$work/video.o" ${extra[@]+"${extra[@]}"} \
  -o "$work/test" $(sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lGL -lz -lm -lpthread

# A data folder of its own: the test writes ajustes.txt and player.txt, and the
# real one must not be touched.
mkdir -p "$work/dados"
NUVIO_DADOS="$work/dados" "$work/test"
