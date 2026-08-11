#!/usr/bin/env bash
# Cross-build fribidi (Unicode bidi) for aarch64-phoenix into /tmp/x11-phoenix.
# Pango dep. NOTE: fribidi's lib/Makefile forces -ansi, which breaks on Phoenix
# sys/wait.h's bare `static inline` ("unknown type name 'inline'"); override with
# -std=gnu11. Verified 2026-08-11: libfribidi.a built + staged.
# Usage: tools/x11-port/build-fribidi.sh
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TC=${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-
SYSROOT=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot
PREFIX=/tmp/x11-phoenix; SRC=${ROOT}/tools/x11-port/src; VER=1.0.13; NV=fribidi-$VER
[ -f "$SRC/$NV.tar.xz" ] || wget -q -O "$SRC/$NV.tar.xz" "https://github.com/fribidi/fribidi/releases/download/v$VER/$NV.tar.xz"
[ -d "$SRC/$NV" ] || tar -C "$SRC" -xf "$SRC/$NV.tar.xz"
cd "$SRC/$NV"
CC="${TC}gcc --sysroot=$SYSROOT" AR="${TC}ar" RANLIB="${TC}ranlib" CFLAGS="-O2" \
  ./configure --host=aarch64-phoenix --prefix="$PREFIX" --disable-shared --enable-static --disable-docs
make -j"$(nproc)" CFLAGS="-O2 -std=gnu11"   # -std=gnu11 overrides fribidi's -ansi (Phoenix header compat)
make install
echo "=== fribidi staged -> $PREFIX/lib/libfribidi.a ==="
