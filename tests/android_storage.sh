#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc -D_POSIX_C_SOURCE=200809L -DNV_CACHE_TEST -Isrc tests/android_storage.c src/androidcache.c -pthread -o "$work/test"
"$work/test" "$work"
