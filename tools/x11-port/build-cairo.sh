#!/usr/bin/env bash
# Cross-build cairo (2D graphics) for aarch64-phoenix into the shared X11 prefix
# /tmp/x11-phoenix, against the already-ported pixman/freetype/fontconfig/libpng
# from build-x11-phoenix.sh. Cairo is the first big step of the GTK/XFce
# dependency chain (glib [done] -> cairo -> pango -> gdk-pixbuf -> gtk).
#
# Builds ONLY the core library (src/) + installs libcairo.a + headers + cairo.pc
# to the prefix. The util/cairo-gobject subdir is skipped: it fails on a
# Phoenix-gcc flag quirk (`-pthread` unrecognized; Phoenix uses `-fpthread`) and
# is only needed for GObject/cairo integration (a later GTK-time follow-up).
#
# Backends: image + freetype(ft) + fontconfig(fc) + png. xlib/xcb/gl/ps/pdf/svg
# are OFF for now (xlib will be needed for GTK-on-X later; enable then with the
# X11 prefix's libX11/libXrender).
#
# Verified 2026-08-11: libcairo.a (1.18 MB) with cairo_create/
# cairo_image_surface_create/cairo_ft_font_face_create/cairo_paint/cairo_fill
# defined, cross-built + installed to the prefix.
#
# Usage: tools/x11-port/build-cairo.sh
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TC=${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-
SYSROOT=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot
PREFIX=/tmp/x11-phoenix
SRC=${ROOT}/tools/x11-port/src
FC=${SRC}/fontconfig-2.14.2         # fontconfig built in-tree (not installed to PREFIX)
VER=1.16.0
NV=cairo-${VER}
URL=https://cairographics.org/releases/${NV}.tar.xz

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig"

[ -d "$PREFIX" ] || { echo "ERROR: run build-x11-phoenix.sh first (need $PREFIX with pixman/freetype/png)"; exit 1; }

# --- fetch + extract ---
[ -f "$SRC/${NV}.tar.xz" ] || wget -q -O "$SRC/${NV}.tar.xz" "$URL"
[ -d "$SRC/$NV" ] || tar -C "$SRC" -xf "$SRC/${NV}.tar.xz"

# --- configure (fontconfig via explicit *_CFLAGS/_LIBS: it has no .pc in PREFIX) ---
cd "$SRC/$NV"
ax_cv_c_float_words_bigendian=no \
FONTCONFIG_CFLAGS="-I$FC" FONTCONFIG_LIBS="-L$FC/src/.libs -lfontconfig" \
CC="${TC}gcc --sysroot=$SYSROOT" AR="${TC}ar" RANLIB="${TC}ranlib" \
CFLAGS="-I$PREFIX/include -I$FC -O2" LDFLAGS="-L$PREFIX/lib -L$FC/src/.libs" \
  ./configure --host=aarch64-phoenix --prefix="$PREFIX" \
    --disable-shared --enable-static \
    --enable-ft --enable-fc --enable-png \
    --disable-xlib --disable-xcb --disable-gl \
    --disable-script --disable-ps --disable-pdf --disable-svg --disable-interpreter

# --- build + install the CORE library only (skip util/ which needs -fpthread) ---
make -C src -j"$(nproc)"
make -C src install
make install-pkgconfigDATA   # cairo.pc

echo "=== cairo core built + staged -> $PREFIX/lib/libcairo.a ==="
"${TC}nm" "$PREFIX/lib/libcairo.a" 2>/dev/null | grep -qE ' T cairo_create$' \
  && echo "[OK] cairo_create defined" || { echo "[FAIL] cairo_create missing"; exit 1; }
