#!/bin/bash
# Verify that no local scripts call docker directly (except allowlisted remote scripts)
set -e

cd "$(dirname "$0")/.."

# Allowlisted scripts that use docker on REMOTE systems (via ssh) - they should not be changed
ALLOWLIST=(
  "tools/painel-issues/api/implantar-remoto.sh"
  "tools/painel-issues/tests/implantar-shim.sh"
)

# Check: no docker calls in local scripts (except in comments and allowlisted files)
found_docker=0
while IFS= read -r line; do
  file=$(echo "$line" | cut -d: -f1)
  
  # Skip allowlisted files
  skip=0
  for allowed in "${ALLOWLIST[@]}"; do
    if [ "$file" = "$allowed" ]; then
      skip=1
      break
    fi
  done
  [ "$skip" = 1 ] && continue
  
  # Skip comment lines
  if echo "$line" | grep -E "^\s*#" >/dev/null; then
    continue
  fi
  
  # Found actual docker call in a non-allowlisted file
  echo "FAIL: Found docker call in $file: $line"
  found_docker=1
done < <(grep -rn "docker " tools tests .claude/skills --include="*.sh" --include="*.md" 2>/dev/null | grep -v "^[^:]*:.*#" || true)

if [ "$found_docker" -eq 0 ]; then
  echo "OK: No direct docker calls in local scripts"
  exit 0
else
  exit 1
fi
