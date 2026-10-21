#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_tmp=$(mktemp -d)
trap 'rm -rf "$task_tmp"' EXIT
python3 - "$task_tmp/test.c" <<'PY'
import pathlib, sys
source = pathlib.Path('src/dts/dts_engine.c').read_text()
start = source.index('    if (', source.index('e->info.dovi_bl_compatibility_id=conf->dv_bl_signal_compatibility_id;'))
end = source.index('    /* mp4toannexb', start)
assert 'attempts=e->allow_p7?RANGE_ATTEMPTS:1' in source
assert 'if(!e->allow_p7) return fail(e,DTS_ENGINE_TIMESTAMP_DISCONTINUITY,0);' in source
assert source.count('if(e->allow_p7)\n') == 3
assert 'dts_playback_next_p7(ajustes_dv7());' in pathlib.Path('src/video.c').read_text()
playback = pathlib.Path('src/dts/dts_playback.c').read_text()
assert 'dts_engine_allow_p7(p->engine, p->allow_p7);' in playback
assert 'target,p->allow_p7 && seek_reload && !stall_seek)' in playback
# Execute the production metadata decision with P7 + a DTS fallback session.
pathlib.Path(sys.argv[1]).write_text(r'''
#include <assert.h>
#include <stdio.h>
#define AV_CODEC_ID_HEVC 1
typedef struct { int dv_profile, el_present_flag, bl_present_flag, rpu_present_flag; } Conf;
typedef struct { int codec_id; } Params;
typedef struct { int dovi_profile, dovi_el_present, dovi_bl_compatibility_id; } Info;
typedef struct { int allow_p7, dv7; Info info; } Engine;
static int nv_dv7_profile(int p,int el,int bl,int rpu) { (void)el; return p==7 && bl && rpu; }
static int fail(Engine *e,const char *message,int code) { (void)e;(void)message;(void)code;return -1; }
static int metadata(Engine *e,Conf *conf,Params *vp) {
''' + source[start:end] + r'''
 return 0;
}
int main(void) {
 Conf conf={7,1,1,1}; Params vp={AV_CODEC_ID_HEVC};
 Engine off={0};
 assert(metadata(&off,&conf,&vp)<0);
 assert(!off.dv7);
 Engine on={.allow_p7=1};
 assert(metadata(&on,&conf,&vp)==0 && on.dv7);
 assert(on.info.dovi_profile==8 && !on.info.dovi_el_present && on.info.dovi_bl_compatibility_id==1);
 puts("DV7 Off + DTS fallback: rejected P7, dv7 remains zero; opt-in converts.");
}
''')
PY
# Check the FFmpeg-enabled branch omitted by the local application build.
cc -std=gnu11 -Wall -Wextra -Werror -Wno-implicit-const-int-float-conversion -DNV_DTS_FFMPEG -DNV_DTS_WEBOS_RECOVERY \
  -DNV_DV7_CONVERSION $(pkg-config --cflags libavformat libavcodec libswresample libavutil) \
  -fsyntax-only src/dts/dts_engine.c
cc -std=c11 -Wall -Wextra -Werror "$task_tmp/test.c" -o "$task_tmp/test"
"$task_tmp/test"
# Run the controller with the opt-in disabled, then without webOS recovery.
cc -std=c11 -D_GNU_SOURCE -Wall -Wextra -Werror -Isrc \
  -D__real_clock_gettime=clock_gettime -c tests/dts_playback.c -o "$task_tmp/fixture.o"
cc -std=c11 -D_GNU_SOURCE -DNV_DTS_WEBOS_RECOVERY -Wall -Wextra -Werror -Isrc \
  -Dclock_gettime=__wrap_clock_gettime "$task_tmp/fixture.o" \
  src/dts/dts_playback.c src/js.c src/linguas.c -pthread -lm -o "$task_tmp/controller"
DV7_OFF=1 "$task_tmp/controller" timestamp
"$task_tmp/controller" timestamp
cc -std=c11 -D_GNU_SOURCE -Wall -Wextra -Werror -Isrc \
  -Dclock_gettime=__wrap_clock_gettime "$task_tmp/fixture.o" \
  src/dts/dts_playback.c src/js.c src/linguas.c -pthread -lm -o "$task_tmp/controller"
RECOVERY_OFF=1 "$task_tmp/controller" timestamp
