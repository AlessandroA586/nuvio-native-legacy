#!/bin/bash
# Build and capture the real folder hero with details on/off in both layouts.
set -eu
cd "$(dirname "$0")/.."
NV_COL=1 NV_DINPALCO_AUDIT=1 NV_FOLDER_DETAILS=1 \
  bash tests/homelayouts_shot.sh /Volumes/ExternalSSD/tmp/rev/pasta-titulo-png 01
