#!/usr/bin/env bash
# Cross-compile llama2.c for Phoenix-RTOS / aarch64 (Pi 4).
# Single-threaded, static, -lm. See README.md.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN="${TOOLCHAIN:-/home/houp/phoenix-rpi/.toolchain/aarch64-phoenix/bin}"
CC="${CC:-$TOOLCHAIN/aarch64-phoenix-gcc}"

"$CC" -O3 -static -o "$HERE/run-llama2" "$HERE/run.c" -lm
echo "built: $HERE/run-llama2"
"$TOOLCHAIN/aarch64-phoenix-size" "$HERE/run-llama2" 2>/dev/null || true
