#!/bin/bash
# On the host: the owner's stop flag reaches the real transport (rede_fluxo64)
# while the origin has sent no byte. Needs FFmpeg dev libs (pkg-config) and libcurl.
set -eu
cd "$(dirname "$0")/.."
DIR=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-dts-cancel.XXXXXX")
PID=''
trap 'if [ -n "$PID" ]; then kill "$PID" 2>/dev/null || true; fi; rm -rf "$DIR"' EXIT
python3 - "$DIR/port" <<'PY' &
import socket, sys
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', 0)); s.listen(8)
open(sys.argv[1], 'w').write(str(s.getsockname()[1]))
held = []
while True:
    c, _ = s.accept(); held.append(c)   # accept, read nothing, answer nothing
PY
PID=$!
for _ in $(seq 50); do [ -s "$DIR/port" ] && break; sleep 0.1; done
PORT=$(cat "$DIR/port")
FF=$(pkg-config --cflags --libs libavformat libavcodec libswresample libavutil)
${CC:-cc} -std=gnu11 -O1 -g -DNV_DTS_FFMPEG tests/dts_engine_cancel.c src/dts/dts_engine.c src/rede.c src/redeurl.c \
  $FF -lm -lpthread -ldl -o "$DIR/test" 2>"$DIR/cc.log" || { cat "$DIR/cc.log"; exit 1; }
"$DIR/test" "http://127.0.0.1:$PORT/source.mkv"
${CC:-cc} -std=gnu11 -O1 -g -DNV_DTS_FFMPEG -DDTS_FAKE_TRANSPORT tests/dts_engine_cancel.c src/dts/dts_engine.c \
  $FF -lm -lpthread -o "$DIR/fake" 2>"$DIR/cc.log" || { cat "$DIR/cc.log"; exit 1; }
"$DIR/fake" "http://blocked.invalid/source.mkv"
