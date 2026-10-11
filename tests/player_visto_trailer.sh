#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-visto-trailer.XXXXXX")
trap 'rm -rf "$dir"' EXIT
export NUVIO_DADOS="$dir/dados"
mkdir -p "$NUVIO_DADOS"
flags=(-O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
  -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
renomes=()
for nome in tocar parar pausar buscar pausa_confirmada tocando pronto ativo falhou conflito_recurso pos duracao url_atual bufferando_ms iniciar_auto terminou; do
  renomes+=("-Dvideo_$nome=nv_base_video_$nome")
done
cc "${flags[@]}" "${renomes[@]}" -c src/video.c -o "$dir/video.o"
# Os DESTINOS da marcacao tambem viram stubs (mesmo truque dos renomes do
# video): o teste conta as chamadas a trakt_marcar e visto_titulo — nenhum
# token, nenhuma rede.
cc "${flags[@]}" -Dtrakt_marcar=nv_base_trakt_marcar -c src/trakt.c -o "$dir/trakt.o"
cc "${flags[@]}" -Dvisto_titulo=nv_base_visto_titulo -Dvisto_episodios=nv_base_visto_episodios -c src/visto.c -o "$dir/visto.o"
# Relogio controlado so no player: nao intercepta libc/SDL/sanitizadores.
cc "${flags[@]}" -DNV_SHOT_HOOKS -Dclock_gettime=nv_test_clock_gettime -c src/player.c -o "$dir/player.o"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/video.c|src/trakt.c|src/visto.c|src/trailer.c|src/player.c) continue;; esac
  sources+=("$source")
done
cc "${flags[@]}" -DNV_SHOT_HOOKS "${sources[@]}" tests/player_visto_trailer.c "$dir/video.o" "$dir/trakt.o" "$dir/visto.o" "$dir/player.o" \
  -o "$dir/test" -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL
"$dir/test"
