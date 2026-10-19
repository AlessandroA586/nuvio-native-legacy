#!/bin/bash
# Offline diagnostic APK build; never source signing or typesafe configuration.
set -eu
cd "$(dirname "$0")/.."
export JAVA_HOME="${JAVA_HOME:-$HOME/.local/jdks/jdk-17.0.20.1+1/Contents/Home}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_SDK_ROOT="$ANDROID_HOME"
unset NUVIO_KEYSTORE
android/gradlew -p android --offline --console=plain --max-workers=2 \
  -Pnuvio.ffmpegAar="$PWD/build/android/media3-ffmpeg-16kb.aar" assembleDebug
