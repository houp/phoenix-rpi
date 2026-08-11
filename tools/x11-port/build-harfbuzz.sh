#!/usr/bin/env bash
# Cross-build harfbuzz (text shaping) for aarch64-phoenix into /tmp/x11-phoenix.
# Pango dep (glib->cairo->[harfbuzz,fribidi]->pango->gtk chain toward XFce).
# Minimal: freetype backend; glib/icu/cairo integration OFF (pango pulls glib
# itself; harfbuzz's own unicode funcs suffice for shaping). C++ (needs g++).
# Verified 2026-08-11: libharfbuzz.a (1.55 MB), hb_shape defined, staged.
# Usage: tools/x11-port/build-harfbuzz.sh   (after build-x11-phoenix.sh + build-cairo.sh)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TC=${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-
SYSROOT=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot
PREFIX=/tmp/x11-phoenix; SRC=${ROOT}/tools/x11-port/src; VER=2.6.7; NV=harfbuzz-$VER
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig"
[ -f "$SRC/$NV.tar.xz" ] || wget -q -O "$SRC/$NV.tar.xz" "https://github.com/harfbuzz/harfbuzz/releases/download/$VER/$NV.tar.xz"
[ -d "$SRC/$NV" ] || tar -C "$SRC" -xf "$SRC/$NV.tar.xz"
cd "$SRC/$NV"
CC="${TC}gcc --sysroot=$SYSROOT" CXX="${TC}g++ --sysroot=$SYSROOT" AR="${TC}ar" RANLIB="${TC}ranlib" \
  CFLAGS="-I$PREFIX/include -O2" CXXFLAGS="-I$PREFIX/include -O2" LDFLAGS="-L$PREFIX/lib" \
  ./configure --host=aarch64-phoenix --prefix="$PREFIX" --disable-shared --enable-static \
    --with-freetype=yes --with-glib=no --with-gobject=no --with-icu=no --with-cairo=no
make -C src -j"$(nproc)"
make -C src install
echo "=== harfbuzz staged; checking hb_shape ==="
"${TC}nm" "$PREFIX/lib/libharfbuzz.a" | grep -qE ' T hb_shape$' && echo "[OK] hb_shape"
