#!/usr/bin/env bash
# Cross-build pango 1.42.4 (text layout/shaping engine) for aarch64-phoenix into
# /tmp/x11-phoenix. The 5th GTK/XFce dep (glib->cairo->harfbuzz+fribidi->PANGO).
# Backends: Cairo + FreeType (libpango/libpangocairo/libpangoft2-1.0.a).
# Prereqs: build-x11-phoenix.sh, build-cairo.sh, build-harfbuzz.sh (WITH glib),
#          build-fribidi.sh, and glib built into the buildroot sysroot + /tmp/phoenix-glib.
#
# CROSS-BUILD LEARNINGS baked in here (each cost a debug cycle 2026-08-11):
#  1. PKG_CONFIG_LIBDIR (not PATH) — REPLACES the host default so host libs
#     (libthai!) aren't picked up. pango auto-enables Thai if it finds host
#     libthai -> break-thai.c fails (no thai/thwchar.h in the cross sysroot).
#  2. cross .pc for fontconfig + libpng16 (cairo.pc Requires them) MUST include a
#     `Description:` line or pkg-config silently skips them -> cairo "not found".
#  3. harfbuzz MUST be built --with-glib=yes: pango's pangofc-shape.c needs
#     <hb-glib.h>.
#  4. cairo.la's dependency_libs had a bad `//lib/libfontconfig.la` (empty libdir)
#     -> libtool link of libpangocairo fails; rewrite to `-lfontconfig`.
# Verified 2026-08-11: libpango-1.0.a (480K, pango_layout_new) + libpangocairo +
# libpangoft2, installed to the prefix.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TC=${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-
SYSROOT=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot
PREFIX=/tmp/x11-phoenix; SRC=${ROOT}/tools/x11-port/src
GLIBPC=${ROOT}/tools/ports/src/glib-2.56.4; FC=${SRC}/fontconfig-2.14.2; PNG=${SRC}/libpng-1.6.40
VER=1.42.4; NV=pango-$VER
export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig:$GLIBPC"   # host-isolated (learning #1)

# (learning #2) ensure fontconfig.pc + libpng16.pc exist in the prefix WITH Description
[ -f "$PREFIX/lib/pkgconfig/fontconfig.pc" ] || printf 'prefix=%s\nincludedir=${prefix}\nlibdir=${prefix}/src/.libs\n\nName: Fontconfig\nDescription: Font configuration library\nVersion: 2.14.2\nRequires: freetype2\nLibs: -L${libdir} -lfontconfig\nCflags: -I${includedir}\n' "$FC" > "$PREFIX/lib/pkgconfig/fontconfig.pc"
[ -f "$PREFIX/lib/pkgconfig/libpng16.pc" ] || { printf 'prefix=%s\nincludedir=${prefix}\nlibdir=${prefix}/.libs\n\nName: libpng\nDescription: PNG library\nVersion: 1.6.40\nLibs: -L${libdir} -lpng16\nCflags: -I${includedir}\n' "$PNG" > "$PREFIX/lib/pkgconfig/libpng16.pc"; ln -sf libpng16.pc "$PREFIX/lib/pkgconfig/libpng.pc"; }
# (learning #4) fix cairo.la's bad //lib/*.la deps so libtool can link pangocairo
for la in "$PREFIX/lib/libcairo.la" "$SRC/cairo-1.16.0/src/libcairo.la"; do
  [ -f "$la" ] && sed -i 's#//lib/libfontconfig.la#-lfontconfig#g; s#//lib/libfreetype.la#-lfreetype#g; s#//lib/libpng16.la#-lpng16#g; s#//lib/libpixman-1.la#-lpixman-1#g' "$la"
done

[ -f "$SRC/$NV.tar.xz" ] || wget -q -O "$SRC/$NV.tar.xz" "https://download.gnome.org/sources/pango/1.42/$NV.tar.xz"
[ -d "$SRC/$NV" ] || tar -C "$SRC" -xf "$SRC/$NV.tar.xz"
cd "$SRC/$NV"
CC="${TC}gcc --sysroot=$SYSROOT" CXX="${TC}g++ --sysroot=$SYSROOT" AR="${TC}ar" RANLIB="${TC}ranlib" \
  CFLAGS="-I$PREFIX/include -O2" LDFLAGS="-L$PREFIX/lib" \
  ./configure --host=aarch64-phoenix --prefix="$PREFIX" --disable-shared --enable-static \
    --with-cairo --without-xft --disable-introspection --disable-gtk-doc
make -C pango -j"$(nproc)"
make -C pango install
echo "=== pango staged; checking pango_layout_new ==="
"${TC}nm" "$PREFIX/lib/libpango-1.0.a" | grep -qE ' T pango_layout_new$' && echo "[OK] pango_layout_new"
