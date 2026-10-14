#!/bin/bash
# Builds rust/mkvcore as a static library and prints the path of the .a.
# Only used with -DNV_RUST_MKVCORE (off by default; docs/plans/rust-mapa.md).
#   tools/rust-mkvcore.sh [rust-target]      e.g. aarch64-linux-android
# Toolchain: RUSTUP_HOME/CARGO_HOME default to the external SSD install. Never
# installs anything: a missing cargo or target is an error.
set -eu
cd "$(dirname "$0")/.."
export RUSTUP_HOME="${RUSTUP_HOME:-/Volumes/ExternalSSD/rust/rustup}"
export CARGO_HOME="${CARGO_HOME:-/Volumes/ExternalSSD/rust/cargo}"
export PATH="$CARGO_HOME/bin:$PATH"
command -v cargo >/dev/null || { echo "rust-mkvcore: cargo not found" >&2; exit 1; }
alvo="${1:-}"
args=(--release --manifest-path rust/mkvcore/Cargo.toml --target-dir "${NV_RUST_TARGET_DIR:-rust/mkvcore/target}")
[ -n "$alvo" ] && args+=(--target "$alvo")
cargo build -q "${args[@]}" >&2
dir="${NV_RUST_TARGET_DIR:-rust/mkvcore/target}"
if [ -n "$alvo" ]; then echo "$(pwd)/$dir/$alvo/release/libmkvcore.a"; else echo "$(pwd)/$dir/release/libmkvcore.a"; fi
