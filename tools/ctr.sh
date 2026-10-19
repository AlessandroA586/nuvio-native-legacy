#!/bin/bash
# Helper to map docker to Apple Container CLI (container)
# Source this file to use: ctr_run, ctr_build, ctr_image_exists, ctr_image_inspect

set -e

# Allow override to docker via NUVIO_CTR=docker for debugging
: "${NUVIO_CTR:=container}"

# Check system is ready
ctr_check_system() {
  if [ "$NUVIO_CTR" = "docker" ]; then
    docker info >/dev/null 2>&1 || { echo "Docker not running" >&2; return 1; }
    return 0
  fi
  # container requires system to be running
  if ! container system status >/dev/null 2>&1; then
    echo "Apple Container system not running. Start with: container system start" >&2
    return 1
  fi
}

# Map docker --platform to --arch
_ctr_platform_to_arch() {
  case "$1" in
    linux/arm64)         echo "arm64" ;;
    linux/amd64)         echo "amd64" ;;
    linux/arm/v5|linux/arm/v7) echo "arm64" ;; # ARM v5/v7 compat with ARMv8
    *)                   echo "arm64" ;; # default for Apple Silicon
  esac
}

# ctr_run [OPTIONS] IMAGE COMMAND [ARGS...]
# Maps: docker run → container run
# --platform linux/X → --arch X
# --rm → --rm (same)
# -v HOST:CONTAINER → -v HOST:CONTAINER (same)
# -v HOST:CONTAINER:ro → -v HOST:CONTAINER (same, read-only not supported, will warn)
# -w /path → -w /path (same)
# -e VAR=VAL → -e VAR=VAL (same)
# --env-file → read and expand
# --network none → not supported, warn
# --pull → not mapped
# --user → not supported, warn
# --entrypoint → not supported, must be handled differently
ctr_run() {
  ctr_check_system || return 1
  
  local arch="arm64" pull="" rm_flag="" vols=() envs=() workdir="" image="" cmd=()
  local collect_cmd=0 i=0 args=("$@")
  
  while [ $i -lt ${#args[@]} ]; do
    local arg="${args[$i]}"
    case "$arg" in
      --platform)
        arch=$(_ctr_platform_to_arch "${args[$((i+1))]}")
        i=$((i+2))
        ;;
      --rm)
        rm_flag="--rm"
        i=$((i+1))
        ;;
      -v|--volume)
        vols+=("-v" "${args[$((i+1))]}")
        i=$((i+2))
        ;;
      -e|--env)
        envs+=("-e" "${args[$((i+1))]}")
        i=$((i+2))
        ;;
      -w|--workdir)
        workdir="-w ${args[$((i+1))]}"
        i=$((i+2))
        ;;
      --pull)
        i=$((i+2))
        ;;
      --env-file)
        # Read file and add as -e flags
        local envfile="${args[$((i+1))]}"
        while IFS= read -r line; do
          [ -z "$line" ] || [[ "$line" =~ ^# ]] && continue
          envs+=("-e" "$line")
        done < "$envfile"
        i=$((i+2))
        ;;
      --network)
        if [ "${args[$((i+1))]}" = "none" ]; then
          echo "WARNING: --network none not supported by container CLI, ignoring" >&2
        fi
        i=$((i+2))
        ;;
      --user|--entrypoint)
        echo "WARNING: $arg not supported by container CLI, ignoring ${args[$((i+1))]}" >&2
        i=$((i+2))
        ;;
      --memory|--cpus)
        # Pass through
        vols+=("$arg" "${args[$((i+1))]}")
        i=$((i+2))
        ;;
      -*)
        echo "Unknown docker flag: $arg" >&2
        return 1
        ;;
      *)
        # First non-flag is image, rest is command
        image="$arg"
        shift $((i+1))
        cmd=("$@")
        break
        ;;
    esac
  done
  
  if [ -z "$image" ]; then
    echo "ctr_run: no image specified" >&2
    return 1
  fi
  
  if [ "$NUVIO_CTR" = "docker" ]; then
    # Fallback to docker (for testing/debugging)
    docker run --rm --platform "$(
      case "$arch" in
        arm64) echo "linux/arm64" ;;
        amd64) echo "linux/amd64" ;;
      esac
    )" "${vols[@]}" "${envs[@]}" $workdir "$image" "${cmd[@]}"
  else
    # Use container
    container run --rm --arch "$arch" "${vols[@]}" "${envs[@]}" $workdir "$image" "${cmd[@]}"
  fi
}

# ctr_build [OPTIONS] -t TAG -f DOCKERFILE CONTEXT
ctr_build() {
  ctr_check_system || return 1
  
  local arch="arm64" tag="" dockerfile="Dockerfile" context="." args=("$@")
  local i=0
  
  while [ $i -lt ${#args[@]} ]; do
    local arg="${args[$i]}"
    case "$arg" in
      --platform)
        arch=$(_ctr_platform_to_arch "${args[$((i+1))]}")
        i=$((i+2))
        ;;
      -t|--tag)
        tag="${args[$((i+1))]}"
        i=$((i+2))
        ;;
      -f|--file)
        dockerfile="${args[$((i+1))]}"
        i=$((i+2))
        ;;
      -*)
        echo "Unknown docker build flag: $arg" >&2
        return 1
        ;;
      *)
        # First non-flag is context
        context="$arg"
        i=$((i+1))
        ;;
    esac
  done
  
  if [ -z "$tag" ]; then
    echo "ctr_build: -t TAG required" >&2
    return 1
  fi
  
  if [ "$NUVIO_CTR" = "docker" ]; then
    docker build --platform "$(
      case "$arch" in
        arm64) echo "linux/arm64" ;;
        amd64) echo "linux/amd64" ;;
      esac
    )" -t "$tag" -f "$dockerfile" "$context"
  else
    container build --arch "$arch" -t "$tag" -f "$dockerfile" "$context"
  fi
}

# ctr_image_exists IMAGE_NAME
# Returns 0 if image exists, 1 otherwise
ctr_image_exists() {
  if [ "$NUVIO_CTR" = "docker" ]; then
    docker image inspect "$1" >/dev/null 2>&1
  else
    container image inspect "$1" >/dev/null 2>&1
  fi
}

# ctr_image_inspect IMAGE_NAME [FORMAT]
# Returns image info; if FORMAT given, tries to apply it
ctr_image_inspect() {
  local image="$1" format="${2:-{{.Id}}}"
  
  if [ "$NUVIO_CTR" = "docker" ]; then
    docker image inspect --format "$format" "$image"
  else
    # container image inspect returns JSON; extract id if requested
    if [ "$format" = "{{.Id}}" ]; then
      container image inspect "$image" 2>/dev/null | grep -o '"id"\s*:\s*"[^"]*' | head -1 | cut -d'"' -f4 || echo ""
    else
      container image inspect "$image" 2>/dev/null
    fi
  fi
}

# ctr_image_list
ctr_image_list() {
  if [ "$NUVIO_CTR" = "docker" ]; then
    docker image ls
  else
    container image ls
  fi
}
