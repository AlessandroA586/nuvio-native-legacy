#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc -D_POSIX_C_SOURCE=200809L -DNV_CACHE_TEST -Isrc tests/android_storage.c src/androidcache.c -pthread -o "$work/test"
cc -O1 -g -Wall -Wextra -DNV_ANDROID -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined src/p2pmotor.c src/p2pmotor_motor.c \
  src/p2p.c src/js.c tests/p2pmotor_teto.c -pthread -o "$work/p2p"
rc=0
"$work/test" "$work" || rc=1
"$work/p2p" || rc=1
exit "$rc"
