#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-video-android-retomada.XXXXXX")
trap 'rm -rf "$dir"' EXIT
jdk=$(/usr/libexec/java_home)
flags=(-O1 -g -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
  -I"$jdk/include" -I"$jdk/include/darwin" -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" -ffunction-sections -fdata-sections tests/video_android_retomada.c \
  src/capmkv.c src/mkv.c tests/capmkv_stub.c \
  -Wl,-dead_strip -lpthread -o "$dir/test"
"$dir/test"
