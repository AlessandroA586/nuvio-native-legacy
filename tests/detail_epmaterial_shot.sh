#!/bin/bash
# Offline A/B/C material comparison, including the unchanged baseline.
set -eu
export NV_EP_MATERIAL_SHOTS=1
out="${1:-/Volumes/ExternalSSD/tmp/rev/epcor}"
mkdir -p "$out"
bash "$(dirname "$0")/detail_eps_shot.sh" "$out"
