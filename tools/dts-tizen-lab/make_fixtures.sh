#!/bin/sh
# Synthetic test media for the DTS/TrueHD -> AC3 lab and the S95C test pass.
# Needs an ffmpeg CLI with libx265 and the experimental dca/truehd encoders.
#
#   sh tools/dts-tizen-lab/make_fixtures.sh OUTDIR [SIZE] [SECONDS]
#      SIZE 1920x1080 (default, host tests) or 3840x2160 (TV: 4K HDR10)
#
# Every audio track is tone-per-channel (FL 300, FR 500, FC 700, LFE 60,
# SL 1300, SR 1500 Hz) so a wrong channel map is audible and measurable;
# "beep" files carry a 50 ms 1 kHz beep on FL/FR/FC every 10 s (A/V sync).
# The video is HEVC Main10 BT.2020/PQ with HDR10 SEI (mastering + MaxCLL),
# open GOP with B-frames, keyframe every 48 frames (2 s).
# Limits of FFmpeg's encoders: no DTS-HD MA (XLL), no 7.1 DTS/TrueHD, no Atmos.
# Those need real files (see README, "Real files").
set -eu
OUT=${1:?outdir}; SIZE=${2:-1920x1080}; DUR=${3:-30}
mkdir -p "$OUT"; cd "$OUT"
Q="-hide_banner -loglevel error -y"
ffmpeg $Q -f lavfi -i "testsrc2=size=$SIZE:rate=24000/1001:duration=$DUR" \
  -vf format=yuv420p10le -c:v libx265 -preset ultrafast -profile:v main10 \
  -x265-params "keyint=48:min-keyint=48:scenecut=0:colorprim=bt2020:transfer=smpte2084:colormatrix=bt2020nc:master-display=G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1):max-cll=1000,400:hdr10=1:hdr10-opt=1:repeat-headers=1:log-level=error" \
  -color_primaries bt2020 -color_trc smpte2084 -colorspace bt2020nc video.mkv
T='0.2*sin(2*PI*300*t)|0.2*sin(2*PI*500*t)|0.2*sin(2*PI*700*t)|0.2*sin(2*PI*60*t)|0.2*sin(2*PI*1300*t)|0.2*sin(2*PI*1500*t)'
B='if(between(mod(t,10),0,0.05),0.5*sin(2*PI*1000*t),0)'
ffmpeg $Q -f lavfi -i "aevalsrc=$T:s=48000:d=$DUR:c=5.1" -c:a pcm_s24le tones51.wav
ffmpeg $Q -f lavfi -i "aevalsrc='$B|$B|$B|0|0|0':s=48000:d=$DUR:c=5.1" -c:a pcm_s24le beep51.wav
ffmpeg $Q -i tones51.wav -c:a dca -strict -2 -b:a 1509000 a_dts51.mka
ffmpeg $Q -i tones51.wav -c:a truehd -strict -2 a_truehd51.mka
ffmpeg $Q -i tones51.wav -c:a ac3 -b:a 640k a_ac3_51.mka
ffmpeg $Q -i beep51.wav -c:a dca -strict -2 -b:a 1509000 a_beep_dts.mka
ffmpeg $Q -i beep51.wav -c:a truehd -strict -2 a_beep_thd.mka
printf '1\n00:00:01,000 --> 00:00:03,000\nSubtitle one\n\n2\n00:00:20,000 --> 00:00:22,000\nSubtitle two\n' > sub.srt
for a in dts51 truehd51 ac3_51; do
  ffmpeg $Q -i video.mkv -i a_$a.mka -i sub.srt -map 0:v -map 1:a -map 2:s -c copy -metadata:s:a:0 language=eng src_$a.mkv
done
ffmpeg $Q -i video.mkv -i a_beep_dts.mka -map 0:v -map 1:a -c copy src_beep_dts.mkv
ffmpeg $Q -i video.mkv -i a_beep_thd.mka -map 0:v -map 1:a -c copy src_beep_thd.mkv
# DTS first (default) + an AC3 track in the same language: must stay native.
ffmpeg $Q -i video.mkv -i a_dts51.mka -i a_ac3_51.mka -map 0:v -map 1:a -map 2:a -c copy \
  -metadata:s:a:0 language=eng -metadata:s:a:1 language=eng src_dual.mkv
rm -f tones51.wav beep51.wav
ls -1 src_*.mkv
