#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
bin=$(mktemp /tmp/nuvio-sinopse.XXXXXX)
trap 'rm -f "$bin"' EXIT
cc -Isrc -x c -o "$bin" - <<'C'
#include "layout.h"
#include <assert.h>
int main(void) {
  assert(NV_FT_HERO_SIN == 22);
  assert(NV_LD_HERO_SIN == 30);
  return 0;
}
C
"$bin"
