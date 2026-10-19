#!/bin/bash
# Focused 2.0.7 visual regression: all languages, scales, frost and D-pad.
set -euo pipefail
cd "$(dirname "$0")/.."
export NUVIO_SHOT_SO=showoff NUVIO_SHOT_FONTE=3
bash tests/novidades_cartao_shot.sh "${1:-/Volumes/ExternalSSD/tmp/rev/novidades207}"
