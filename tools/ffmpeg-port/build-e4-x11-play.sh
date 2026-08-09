#!/usr/bin/env bash
#
# Phoenix-RTOS — build the E4 windowed video player (e4_x11_play.c) as a single
# static aarch64-phoenix ELF.
#
# The player decodes a raw Annex-B H.264 clip with the ALREADY-BUILT ffmpeg
# decode-core archives and presents each frame into an X11 window under the
# Xphoenix server (instead of fullscreen /dev/fb0).
#
# Reuses the ALREADY-BUILT static archives — this script only compiles the one new
# TU and links it; it does NOT rebuild ffmpeg / X / libphoenix.
#   - ffmpeg decode core:  external/ffmpeg/libav{codec,format,util}.a
#   - X client stack:      /tmp/x11-phoenix/lib/lib{X11,xcb,Xau,Xdmcp}.a
#   - fresh libc:          .buildroot/.../lib/libphoenix.a (has the new libm ffmpeg needs)
#
# The compile flags mirror build-ffmpeg-phoenix.py (just -I external/ffmpeg to
# resolve <libavcodec/...> + the generated config.h) plus -I/tmp/x11-phoenix/include
# for the X headers. The link recipe combines build-ffmpeg-phoenix.py's group
# (av libs + fresh libphoenix, -lm -lgcc) with build-gl-x11-window.sh's X libs
# (-lX11 -lxcb -lXau -lXdmcp -liconv) INSIDE one --start-group so cross-refs
# resolve; libX11 references iconv from the sysroot.
#
# Host-side cross-compile only. Idempotent / re-runnable.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright (C) 2026 Phoenix Systems. Author: Witold Bolt.
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")/../.." && pwd)"

TC="${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-gcc"
NM="${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-nm"

FFMPEG="${ROOT}/external/ffmpeg"
AVCODEC="${FFMPEG}/libavcodec/libavcodec.a"
AVFORMAT="${FFMPEG}/libavformat/libavformat.a"
AVUTIL="${FFMPEG}/libavutil/libavutil.a"

# Fresh cross-built libc (has the new libm: erf/exp2/exp2f/log2f/...). The
# toolchain sysroot copy is stale for those; link this one explicitly, first in
# the group, exactly as build-ffmpeg-phoenix.py does.
LIBPHOENIX="${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/lib/libphoenix.a"

XPREFIX="/tmp/x11-phoenix"
SYSROOT="${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot"

SRC="${ROOT}/tools/ffmpeg-port/e4_x11_play.c"
OBJ="/tmp/e4_x11_play.o"
GPU_LIBS="${ROOT}/tools/.gpu-libs"
ELF="${GPU_LIBS}/e4-x11-play"

fail() { echo "FAIL: $*" >&2; exit 1; }

[ -x "$TC" ]         || fail "toolchain gcc not found: $TC"
[ -f "$AVCODEC" ]    || fail "missing $AVCODEC (run build-ffmpeg-phoenix.py first)"
[ -f "$AVFORMAT" ]   || fail "missing $AVFORMAT (run build-ffmpeg-phoenix.py first)"
[ -f "$AVUTIL" ]     || fail "missing $AVUTIL (run build-ffmpeg-phoenix.py first)"
[ -f "$LIBPHOENIX" ] || fail "missing fresh $LIBPHOENIX (./scripts/rebuild-rpi4b-fast.sh --scope core)"
[ -f "$XPREFIX/lib/libX11.a" ] || fail "missing $XPREFIX/lib/libX11.a (run build-x11-phoenix.sh first)"
[ -f "$SRC" ]        || fail "missing source $SRC"

mkdir -p "$GPU_LIBS"

# --- compile -----------------------------------------------------------------
# Mirror build-ffmpeg-phoenix.py's demo compile (-O2 -g -I external/ffmpeg, which
# resolves <libavcodec/...> and the generated config.h), plus the X headers.
echo "=== compiling e4_x11_play.c ==="
"$TC" -c -O2 -g -I "$FFMPEG" -I "$XPREFIX/include" -o "$OBJ" "$SRC" \
	|| fail "compile failed"

# --- link ---------------------------------------------------------------------
# One --start-group: av libs + fresh libphoenix (build-ffmpeg model) + the X client
# libs (build-gl-x11-window model). libX11 <-> xcb/Xau/Xdmcp are mutually
# referential and libX11 references iconv (from the sysroot), so all go inside the
# group; -lm -lgcc trail, matching build-ffmpeg-phoenix.py. No -lpthread (pthread
# lives in libphoenix) and no -lstdc++ (ffmpeg is pure C).
echo "=== linking $ELF ==="
"$TC" -o "$ELF" "$OBJ" \
	-L"$XPREFIX/lib" -L"$SYSROOT/lib" \
	-Wl,--start-group \
		"$AVFORMAT" "$AVCODEC" "$AVUTIL" "$LIBPHOENIX" \
		-lX11 -lxcb -lXau -lXdmcp -liconv \
	-Wl,--end-group \
	-lm -lgcc \
	|| fail "link failed"

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
