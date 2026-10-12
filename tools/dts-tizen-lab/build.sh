#!/bin/sh
# Builds the DTS/TrueHD -> AC3 lab on a PC (Linux or macOS): FFmpeg 7.1.5 with the
# component set PROPOSED for the NuvioTpk build (host target, static, LGPL), then
# ./tvconv against it. Nothing here goes into the app.
#
#   sh tools/dts-tizen-lab/build.sh            # -> tools/dts-tizen-lab/tvconv
#
# Needs: cc, make, curl or git, tar/xz, pkg-config not required.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${NUVIO_LAB_DIR:-$HERE/.build}
VERSION=7.1.5
SHA256=de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f   # same pin as tools/build-dts-ffmpeg.sh
SRC=$OUT/ffmpeg-$VERSION
PREFIX=$OUT/prefix
JOBS=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}
CC=${CC:-cc}
mkdir -p "$OUT"

sha() { if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

if [ ! -f "$PREFIX/lib/libavcodec.a" ]; then
  if [ ! -d "$SRC" ]; then
    if curl -fsSL "https://ffmpeg.org/releases/ffmpeg-$VERSION.tar.xz" -o "$OUT/ffmpeg.tar.xz" 2>/dev/null \
       && [ "$(sha "$OUT/ffmpeg.tar.xz")" = "$SHA256" ]; then
      tar -xf "$OUT/ffmpeg.tar.xz" -C "$OUT"
    else
      echo "release tarball unavailable, using the git tag n$VERSION (GitHub mirror)" >&2
      git clone -q --depth 1 --branch "n$VERSION" https://github.com/FFmpeg/FFmpeg.git "$SRC"
    fi
  fi
  mkdir -p "$OUT/obj" && cd "$OUT/obj"
  # === Proposed .tpk component set (Session 1 copies this list) ===
  "$SRC/configure" --cc="$CC" --disable-x86asm --prefix="$PREFIX" --disable-everything \
    --disable-autodetect --disable-shared --enable-static --enable-pic \
    --disable-programs --disable-doc --disable-debug --disable-network \
    --disable-avdevice --disable-avfilter --disable-swscale --disable-postproc \
    --enable-avcodec --enable-avformat --enable-avutil --enable-swresample \
    --enable-decoder=dca,truehd,mlp,dvdsub,pgssub,ass,ssa,subrip,movtext,webvtt \
    --enable-encoder=aac,ac3 \
    --enable-demuxer=matroska,mov \
    --enable-muxer=mpegts,matroska,mp4 \
    --enable-parser=dca,mlp,h264,hevc,aac,ac3 \
    --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb,dca_core,truehd_core \
    --extra-cflags=-O2 > configure.log 2>&1 || { tail -20 configure.log; exit 1; }
  make -j"$JOBS" > make.log 2>&1 || { tail -30 make.log; exit 1; }
  make install > install.log 2>&1
fi
cd "$HERE"
LIBS="-Wl,--start-group -lavformat -lavcodec -lswresample -lavutil -Wl,--end-group"
[ "$(uname)" = Darwin ] && LIBS="-lavformat -lavcodec -lswresample -lavutil"
$CC -O2 -std=gnu11 -Wall -I"$PREFIX/include" tvconv.c -o tvconv -L"$PREFIX/lib" $LIBS -lm -lpthread
echo "built $HERE/tvconv (FFmpeg $VERSION in $PREFIX)"
