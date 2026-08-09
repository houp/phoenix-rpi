#!/usr/bin/env bash
#
# Build the standalone Bluetooth Tier-0 probe (tools/bt-probe/bt-probe.c) with
# the aarch64-phoenix toolchain. Self-contained: uses only mmap/va2pa/usleep/
# printf. Mirrors tools/wifi-probe/build.sh (no phoenix-rtos-build machinery).
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$HERE/../.." && pwd)"
GCC="${GCC:-$REPO_ROOT/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-gcc}"
NM="${NM:-$REPO_ROOT/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-nm}"
OUT="$HERE/bt-probe"
CFLAGS="-O2 -Wall -Wextra -std=gnu11"

echo "bt-probe: compiling + linking"
"$GCC" $CFLAGS "$HERE/bt-probe.c" -o "$OUT"

echo "bt-probe: undefined-symbol check (expect none):"
undef=$("$NM" -u "$OUT" 2>/dev/null | grep -vE 'GLIBC|^\s*$' | wc -l || true)
echo "  $undef undefined symbols."
echo "bt-probe: done -> $OUT"
file "$OUT"
ls -l "$OUT"
