#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
binary=$(mktemp "${TMPDIR:-/tmp}/nuvio-hdr-surface.XXXXXX")
trap 'rm -f "$binary"' EXIT
cc -std=c99 -Wall -Wextra -Werror tests/hdr_surface.c -o "$binary"
"$binary"
"$binary" missing-symbol
python3 - <<'PY'
from pathlib import Path
main = Path('src/main.c').read_text()
assert main.index('trying FP16') < main.index('trying SDR')
assert 'SDL_GL_FLOATBUFFERS, 1' in main and 'SDL_GL_FLOATBUFFERS, 0' in main
shader = Path('src/gfx.c').read_text()
assert '203.0/80.0' in shader and '0.17883277*log' in shader
java = Path('android/app/src/main/java/space/nuvio/nativelegacy/NuvioActivity.kt').read_text()
for token in ('[hdr-ui] cap=', 'supportedHdrTypes', 'desiredMaxLuminance', 'screen.mode', 'screen.isHdr', 'window.colorMode', 'screen.isWideColorGamut'):
    assert token in java, token
cmake = Path('android/app/src/main/cpp/CMakeLists.txt').read_text()
assert 'nvHdrAfterSwap(egl_surface)' in cmake
print('PASS FP16 retry/reset, HDR transfers, Java capability log, first-swap hook')
PY
# Existing opt-in defaults and device-local persistence regression.
bash tests/ajustes_padroes.sh
