#!/usr/bin/env bash
#
# Phoenix-RTOS — build the D1/D2 "accelerated GL in an X window" harness
# (gl_x11_window.c) as a single static aarch64-phoenix ELF.
#
# The harness renders an animated 3D scene with the V3D GPU into an offscreen FBO,
# reads it back with glReadPixels, and presents it into its own X11 window via
# XPutImage (GPU pixels in a normal window under the Xphoenix server).
#
# Reuses the ALREADY-BUILT static archives — this script only compiles the one new
# TU and links it; it does NOT rebuild Mesa / GL / X.
#   - GL frontend + V3D driver:  tools/.gpu-libs/libGL-phoenix.a, libv3d-phoenix.a
#   - X client stack:            /tmp/x11-phoenix/lib/lib{X11,xcb,Xau,Xdmcp}.a
#
# The mesa include set + defines are the SAME ones build-quakespasm-phoenix.py's
# MFLAGS use to compile the mesa-facing glue (they resolve pipe/*, main/*,
# frontend/*, state_tracker/*, GL/* and avoid u_endian/timespec errors); the link
# recipe follows build-quakespasm-phoenix.py's model (--start-group GL+V3D, 32 MB
# stack), with the X client libs added INSIDE the group so cross-refs resolve.
#
# Host-side cross-compile only. Idempotent / re-runnable.
#
# SPDX-License-Identifier: Zlib
# Copyright 2026 Phoenix Systems. Author: Witold Bołt.
set -u

# Repo root derived from this script's own location (portable across checkouts).
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")/../.." && pwd)"

TC="${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-gcc"
NM="${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-nm"

MESA="${ROOT}/external/mesa"
MESABUILD="/tmp/mesa-v3d-build"
COMPAT="${ROOT}/tools/v3d-driver-port/phoenix_mesa_compat.h"
GPU_LIBS="${ROOT}/tools/.gpu-libs"
GLLIB="${GPU_LIBS}/libGL-phoenix.a"
V3DLIB="${GPU_LIBS}/libv3d-phoenix.a"

XPREFIX="/tmp/x11-phoenix"
SYSROOT="${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot"

SRC="${ROOT}/tools/x11-port/gl_x11_window.c"
OBJ="/tmp/gl_x11_window.o"
ELF="${GPU_LIBS}/gl-x11-window"

fail() { echo "FAIL: $*" >&2; exit 1; }

[ -x "$TC" ]        || fail "toolchain gcc not found: $TC"
[ -f "$GLLIB" ]     || fail "missing $GLLIB (run build-gl-phoenix.py first)"
[ -f "$V3DLIB" ]    || fail "missing $V3DLIB (run build-v3d-phoenix.py first)"
[ -f "$XPREFIX/lib/libX11.a" ] || fail "missing $XPREFIX/lib/libX11.a (run build-x11-phoenix.sh first)"
[ -f "$SRC" ]       || fail "missing source $SRC"

# --- compile -----------------------------------------------------------------
# Mesa-side flags (verbatim from build-quakespasm-phoenix.py MFLAGS) + the X headers.
# The endianness/timespec defines and -include COMPAT are load-bearing (else
# u_endian #error / struct timespec redefinition).
MFLAGS=(-O2 -g -ffreestanding -fno-strict-aliasing -Wno-error -Wno-undef
        -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 -DHAVE_STRUCT_TIMESPEC
        -include "$COMPAT"
        -I"${MESA}/src" -I"${MESA}/include" -I"${MESA}/src/mesa"
        -I"${MESA}/src/mapi" -I"${MESA}/src/compiler"
        -I"${MESA}/src/gallium/include" -I"${MESA}/src/gallium/auxiliary"
        -I"${MESA}/src/util" -I"${MESABUILD}/src"
        -I"${XPREFIX}/include")

echo "=== compiling gl_x11_window.c ==="
"$TC" -c "$SRC" -o "$OBJ" "${MFLAGS[@]}" || fail "compile failed"

# --- link ---------------------------------------------------------------------
# Model = build-quakespasm-phoenix.py: objs, --start-group GL+V3D --end-group,
# -lstdc++ -lm, 32 MB stack. The X client libs go INSIDE the group with GL/V3D so
# any cross-references resolve (libX11 <-> xcb/Xau/Xdmcp are mutually referential).
echo "=== linking $ELF ==="
"$TC" "$OBJ" \
	-L"${XPREFIX}/lib" -L"${SYSROOT}/lib" \
	-Wl,--start-group \
		"$GLLIB" "$V3DLIB" \
		-lX11 -lxcb -lXau -lXdmcp -liconv \
	-Wl,--end-group \
	-lstdc++ -lm \
	-Wl,-z,stack-size=33554432 \
	-o "$ELF" || fail "link failed"

# --- pre-flight ---------------------------------------------------------------
echo "=== PRE-FLIGHT ==="
file "$ELF"
case "$(file "$ELF")" in
	*"ARM aarch64"*"statically linked"*) echo "[OK] aarch64 static ELF" ;;
	*) fail "binary is not an aarch64 static ELF" ;;
esac
und="$("$NM" -u "$ELF" 2>/dev/null)"
if [ -z "$und" ]; then
	echo "[OK] 0 undefined symbols"
else
	echo "--- undefined symbols ---"
	echo "$und"
fi

echo "=== BUILT ELF: $ELF ($(du -h "$ELF" | cut -f1)) ==="
