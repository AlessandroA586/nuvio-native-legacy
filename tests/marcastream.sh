#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra -Isrc tests/marcastream.c -o /tmp/nuvio-marcastream-tests
/tmp/nuvio-marcastream-tests
