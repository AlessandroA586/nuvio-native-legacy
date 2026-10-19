#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
binary=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-hdr-surface.XXXXXX")
testdir="$binary"
trap 'rm -rf "$testdir"' EXIT
python3 - "$binary/summary.inc" <<'PY'
from pathlib import Path
import sys
main = Path('src/main.c').read_text()
start = main.index('  { const char *space = SDL_GetHint("NUVIO_HDR_EGL");')
end = main.index('\n#endif', start)
Path(sys.argv[1]).write_text(main[start:end])
PY
cc -std=c99 -Wall -Wextra -Werror -I"$binary" tests/hdr_surface.c -o "$binary/run"
binary="$binary/run"
"$binary" > "$testdir/log"
cat "$testdir/log"
rg -q 'egl=dataspace config=1010102 resultado=hdr dataspace=pq gl=8/8/8/8 blit=1 white=203' "$testdir/log"
rg -q 'egl=dataspace config=FP16 resultado=hdr dataspace=scrgb-linear.*blit=2' "$testdir/log"
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
assert 'SDL_GL_GetAttribute' not in main[main.index('int ten = config'):main.index('if (hdrPedido) {', main.index('int ten = config'))]
gpu = Path('src/gpunivel.c').read_text()
assert 'gfx_rect(tela, intTex, GFX_HDR_OUTPUT, (float)gfx_hdr_output' in gpu
assert 'c*0.0203' in shader
print('PASS verified EGL config drives summary/blit despite GL 8888; FP16 fallback; no 8-bit PQ')
PY
# Existing opt-in defaults and device-local persistence regression.
bash tests/ajustes_padroes.sh
