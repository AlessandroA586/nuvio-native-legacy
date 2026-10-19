#!/bin/bash
# Verify that no local scripts call docker directly (except allowlisted remote scripts)
set -e

cd "$(dirname "$0")/.."

# Allowlisted scripts/patterns that are OK to call docker:
# 1. tools/ctr.sh - the helper that wraps docker/container
# 2. tools/painel-issues/api/implantar-remoto.sh - runs on REMOTE Linux server
# 3. tools/painel-issues/tests/implantar-shim.sh - test shim for remote
# 4. Comments (lines starting with #)
# 5. Echo statements or help text

check_script() {
  local file="$1"
  local script_name=$(basename "$file")
  
  # Skip allowlisted files
  case "$script_name" in
    ctr.sh) return 0 ;; # helper is allowed
    implantar-remoto.sh) return 0 ;; # remote only
    implantar-shim.sh) return 0 ;; # test shim
    publicar.sh) return 0 ;; # docker calls are remote via ssh
    sem_docker.sh) return 0 ;; # this test itself
  esac
  
  # Check for actual docker calls (not in comments, not in echo)
  # Pattern: \bdocker\s+(run|build|image|ps) (actual commands, not text)
  local lines=$(grep -n "^\s*docker\s\+\(run\|build\|image\|ps\|exec\)" "$file" 2>/dev/null || true)
  if [ -n "$lines" ]; then
    echo "FAIL: $file has direct docker calls:"
    echo "$lines" | sed 's/^/  /'
    return 1
  fi
  return 0
}

found_issues=0
for file in tools/*.sh tools/*/*.sh tests/*.sh; do
  [ -f "$file" ] || continue
  check_script "$file" || found_issues=1
done

if [ "$found_issues" -eq 0 ]; then
  echo "OK: No direct docker calls in local scripts (except helpers and remote)"
  exit 0
else
  exit 1
fi
