#!/bin/bash
# Focused review regressions; no application rebuild required.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tests/textoescala_fix2.py
bash -n tests/textoescala_prova.sh
git diff --check
