#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# O modal de estilo da fileira (lista de formas + ordem da colecao) e a
# fileira de pessoas na forma Redonda, em PNG, sem rede (artes de
# deploy/app/art). Nao entra na suite (testa-tudo.sh pula *_shot.sh).
#
#   bash tests/fileiraui_shot.sh /tmp/nv-fileiraui [prefixo]
set -eu
cd "$(dirname "$0")/.."

saida="${1:-${TMPDIR:-/tmp}/nv-fileiraui}"
pref="${2:-fil}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fileiraui-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
# Nunca executar um binario velho se o compilador falhar.
if ! cc "${sources[@]}" tests/fileiraui_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -g ${NV_CFLAGS:-} -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined \
  >"$NUVIO_DADOS/build.log" 2>&1; then
  cat "$NUVIO_DADOS/build.log" >&2
  exit 1
fi
grep -E "fileiraui|ctxmenu\.c|error" "$NUVIO_DADOS/build.log" || true
"$NUVIO_DADOS/shot" "$saida/$pref"
for f in "$saida/$pref"-*.bmp; do
  [ -e "$f" ] || continue
  sips -s format png "$f" --out "${f%.bmp}.png" >/dev/null 2>&1 && rm -f "$f"
done
