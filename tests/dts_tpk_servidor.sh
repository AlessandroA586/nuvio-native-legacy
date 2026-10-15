#!/bin/bash
# The Samsung DTS loopback server (src/dts/dts_hls_tpk.c) for real on the host:
# FFmpeg built with the .tpk component set (NUVIO_DTS_TARGET=tpk
# tools/build-dts-ffmpeg.sh, prefix in NUVIO_DTS_ROOT, default /opt/nuvio-dts),
# fixtures made by the system ffmpeg CLI (libx264 + the experimental DTS
# encoder), source served by tests/dts_tpk_servidor_origem.py.
set -eu
cd "$(dirname "$0")/.."
ROOT=${NUVIO_DTS_ROOT:-/opt/nuvio-dts}
if [ ! -f "$ROOT/lib/libavformat.a" ] || ! command -v ffmpeg >/dev/null || ! command -v ffprobe >/dev/null; then
  echo "dts_tpk_servidor: skipped (needs FFmpeg for the .tpk in $ROOT and the ffmpeg/ffprobe CLI)"
  exit 0
fi
work=$(mktemp -d /tmp/nuvio-dts-tpk-servidor.XXXXXX)
pid=''
trap 'if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || true; fi; rm -rf "$work"' EXIT
mkdir -p "$work/src"
# 30 s, keyframe every 2 s (segments of 6 s), DTS 5.1, Matroska with Cues.
ffmpeg -v error -f lavfi -i testsrc2=s=640x360:r=24 -f lavfi -i "sine=f=440:r=48000" -t 30 \
  -filter_complex "[1:a]pan=5.1|FL=c0|FR=c0|FC=c0|LFE=c0|BL=c0|BR=c0[a]" -map 0:v -map "[a]" \
  -c:v libx264 -preset ultrafast -g 48 -keyint_min 48 -sc_threshold 0 -pix_fmt yuv420p \
  -c:a dca -strict -2 -b:a 768k "$work/src/fx.mkv"
python3 tests/dts_tpk_servidor_origem.py "$work/src" "$work/port" "$work/origem.log" &
pid=$!
for _ in $(seq 50); do [ -s "$work/port" ] && break; sleep 0.1; done
port=$(cat "$work/port")
${CC:-gcc} -O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-misleading-indentation \
  -DNV_TPK -DNV_DTS_FFMPEG -I"$ROOT/include" -Isrc \
  tests/dts_tpk_servidor.c src/dts/dts_hls_tpk.c src/dts/dts_remux.c src/dts/dts_engine.c src/rede.c src/redeurl.c \
  "$ROOT/lib/libavformat.a" "$ROOT/lib/libavcodec.a" "$ROOT/lib/libswresample.a" "$ROOT/lib/libavutil.a" \
  -lz -lm -lpthread -ldl -o "$work/test"
fail=0
dur() { ffprobe -v error -show_entries format=duration -of csv=p=0 "$1"; }
check() { if eval "$2"; then echo "ok   $1"; else echo "FAIL $1"; fail=1; fi; }

# 1. Tuning test from the file: ring 8 MiB, one generator, start tag + short seg0.
mkdir -p "$work/t1"
printf 'ring=8 gens=1 seg0=3\n' > "$work/t1/dts-ajuste.txt"
"$work/test" "$port" tuning "$work/t1" > "$work/t1.log" 2>&1 || fail=1
grep -E '^(ok|FAIL)' "$work/t1.log"
check "tuning: seg0 cut to about 1 s ($(dur "$work/t1/seg0.ts") s)" "awk 'BEGIN{exit !($(dur "$work/t1/seg0.ts") < 2.0)}'"
check "tuning: seg1 whole ($(dur "$work/t1/seg1.ts") s)" "awk 'BEGIN{exit !($(dur "$work/t1/seg1.ts") > 5.5)}'"
check "tuning: seg0 asked again is whole ($(dur "$work/t1/seg0b.ts") s)" "awk 'BEGIN{exit !($(dur "$work/t1/seg0b.ts") > 5.5)}'"
check "tuning: the session line names the tuning" "grep -q 'tuning: ring 8 MiB, 1 generator(s), seg0 start tag + short' '$work/t1.log'"
check "tuning: one memory line at the end" "[ \$(grep -c '^\[dts-tpk\] mem: ' '$work/t1.log') = 1 ]"
check "tuning: memory line has the tuning, RSS, MemAvailable and the rings" \
  "grep -E '^\[dts-tpk\] mem: sessao [0-9]+, .*ring 8 MiB, 1 generator\(s\), seg0 start tag \+ short; app RSS peak [1-9][0-9]* MB .*MemAvailable low [0-9]+ MB .*reader rings peak 1 = 8 MiB' '$work/t1.log' >/dev/null"
check "tuning: only one reader ever opened (one generator)" "! grep -q 'own reader opened' '$work/t1.log'"

# 2. No file: today's behaviour (no start tag, whole seg0, ring 32 MiB here).
mkdir -p "$work/t2"
"$work/test" "$port" padrao "$work/t2" > "$work/t2.log" 2>&1 || fail=1
grep -E '^(ok|FAIL)' "$work/t2.log"
check "default: seg0 whole ($(dur "$work/t2/seg0.ts") s)" "awk 'BEGIN{exit !($(dur "$work/t2/seg0.ts") > 5.5)}'"
check "default: memory line with ring auto and 2 generators" \
  "grep -qE '^\[dts-tpk\] mem: .*ring auto, 2 generator\(s\), seg0 normal; .*reader rings peak [12] = (32|64) MiB' '$work/t2.log'"
check "default: no seg0 cut" "! grep -q 'seg0 cut' '$work/t2.log'"

cp "$work/t1.log" "$work/t2.log" "${DTS_TPK_LOGS:-$work}/" 2>/dev/null || true
if [ "$fail" != 0 ]; then echo "--- t1.log"; grep '\[dts-tpk\]' "$work/t1.log" | tail -30; echo "--- t2.log"; grep '\[dts-tpk\]' "$work/t2.log" | tail -30; fi
[ "$fail" = 0 ] && echo "dts_tpk_servidor: ok"
exit "$fail"
