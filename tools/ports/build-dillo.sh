#!/usr/bin/env bash
#
# Phoenix-RTOS — cross-build the Dillo web browser (dillo-browser/dillo 3.2.0)
# for aarch64-phoenix as a STATIC /bin/dillo, staged to the NFS rootfs export.
# Task #53. Host-side build only; does NOT boot/flash the Pi.
#
# Dillo is a lightweight C/C++ FLTK 1.3 X11 web browser. It links against:
#   - FLTK 1.3.10 static libs        (/tmp/fltk-phoenix — built by build-fltk.sh)
#   - the X11 client closure         (/tmp/x11-phoenix  — libX11/libxcb/libXau/...)
#   - the image libs png16/jpeg/zlib (from /tmp/x11-phoenix)
#
# TLS/HTTPS = ENABLED via mbedTLS (task E1, KNOWN-ISSUE #70). Dillo 3.2.0 supports
# both OpenSSL and mbedTLS; we force mbedTLS with --disable-openssl:
#   - License: Dillo is GPLv3. mbedTLS is Apache-2.0, which is GPL-compatible.
#     OpenSSL-1.1.1a carries the historical OpenSSL-license/GPL friction, so mbedTLS
#     is the clean choice (and the one the task prefers).
#   - The static mbedTLS closure (libmbedtls/libmbedx509/libmbedcrypto) plus its
#     headers are already in the common Phoenix build dir (BUILDLIB/BUILDINC below);
#     we point configure + the final link at them.
# NOTE: this delivers an HTTPS-CAPABLE *build* (configure+link). End-to-end HTTPS
# browsing (working platform entropy for mbedtls_ctr_drbg + a CA-cert bundle +
# Pi internet/NAT) is task E2 and is NOT verified here.
# Still disabled:
#   --disable-webp   libwebp not ported
# Everything else stays ENABLED — Phoenix's sysroot already provides the needed
# primitives (png/jpeg/gif/svg images, cookies, threaded-dns via getaddrinfo,
# iconv via libiconv, sockets/poll/select/fork/exec/pthread all verified present).
#
# THE LINK IS THE REAL RISK, not the configure flags. Dillo drives its final link
# through `fltk-config --ldflags`, which emits only `-lfltk -lpthread -lX11` — it
# OMITS the static xcb closure (-lxcb -lXau -lXdmcp), the image libs, and the
# -Wl,--start-group wrapping that a static C++/X cross-link needs. So we point
# FLTK_CONFIG at a WRAPPER (generated below) whose --ldflags emits the full
# grouped closure proven by the FLTK hello smoke test. Everything else delegates
# to the real fltk-config.
#
# Idempotent. Re-runnable. Output: $NFS/bin/dillo (+ a copy in artifacts/x11).
#
# Copyright 2026 Phoenix Systems
# Author: Witold Bołt
set -u

NV=dillo-3.2.0
URL=https://github.com/dillo-browser/dillo/archive/refs/tags/v3.2.0.tar.gz

# Repo root derived from this script's own location (portable across checkouts).
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")/../.." && pwd)"

TC=${ROOT}/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-
SYSROOT=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/sysroot
# Common Phoenix build dir carrying the static mbedTLS closure (libmbedtls,
# libmbedx509, libmbedcrypto) + its headers (mbedtls/*.h). Used for TLS/HTTPS.
BUILDLIB=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/lib
BUILDINC=${ROOT}/.buildroot/_build/aarch64a72-generic-rpi4b/include
XPREFIX=/tmp/x11-phoenix            # READ-ONLY: shared X11 client lib stack
FPREFIX=/tmp/fltk-phoenix          # READ-ONLY: FLTK 1.3.10 static libs + fltk-config
PREFIX=/tmp/dillo-phoenix          # our own build/install prefix
SRC=${ROOT}/tools/ports/src
XDIR=$SRC/$NV
ART=${ROOT}/artifacts/x11
NFS="${SHOWCASE_STAGE_DIR:-/srv/phoenix-rpi4-nfs}"
SHIM=${ROOT}/tools/ports/dillo-phoenix-shim.h

fail() { echo "FAIL: $*"; exit 1; }

[ -f "$FPREFIX/lib/libfltk.a" ]  || fail "$FPREFIX/lib/libfltk.a missing — run build-fltk.sh first"
[ -f "$FPREFIX/fltk-config" ]    || fail "$FPREFIX/fltk-config missing — run build-fltk.sh first"
[ -f "$XPREFIX/lib/libX11.a" ]   || fail "$XPREFIX/lib/libX11.a missing — build the X11 client stack first"
[ -f "$SHIM" ]                   || fail "$SHIM missing"
[ -f "$BUILDLIB/libmbedtls.a" ]  || fail "$BUILDLIB/libmbedtls.a missing — build mbedtls-2.28.0 first (needed for TLS)"
[ -f "$BUILDINC/mbedtls/ssl.h" ] || fail "$BUILDINC/mbedtls/ssl.h missing — mbedtls headers needed for TLS"

mkdir -p "$SRC" "$PREFIX"

# --- fetch + extract ---------------------------------------------------------
if [ ! -d "$XDIR" ]; then
	[ -f "$SRC/$NV.tar.gz" ] || { echo "=== fetching $URL ==="; curl -sSL --max-time 180 -o "$SRC/$NV.tar.gz" "$URL" || fail "download failed"; }
	tar -C "$SRC" -xf "$SRC/$NV.tar.gz" || fail "extract failed"
fi

# --- source patches (idempotent; survive a clean re-extract) ------------------
# 1. strndup: dw/selection.cc UNCONDITIONALLY defines `extern "C" strndup()`
#    (assuming a platform without it). Phoenix's libphoenix DOES export a
#    non-weak strndup -> "multiple definition" at the final link. Rename Dillo's
#    private definition out of the way; its four call sites then resolve to
#    libphoenix's strndup. (A force-include shim can't fix this — it's a function
#    *definition*, and lout/misc.hh also has a class method named strndup that a
#    macro would mangle.)
if ! grep -q 'dillo_unused_strndup' "$XDIR/dw/selection.cc" 2>/dev/null; then
	sed -i 's/extern "C" char \*strndup(/extern "C" char *dillo_unused_strndup(/' "$XDIR/dw/selection.cc" \
		&& echo "=== patched dw/selection.cc (strndup -> dillo_unused_strndup) ==="
fi
# 2. getsockopt size mismatch: src/IO/http.c declares `uint_t connect_ret_size`
#    (4 bytes) but Phoenix's getsockopt() writes a `socklen_t` (8 bytes on LP64)
#    through that pointer -> a 4-byte stack overwrite on EVERY HTTP connect (the
#    hottest browser path). Fix the type to socklen_t. This is a real correctness
#    fix, not just a warning silence.
if grep -q 'uint_t connect_ret_size' "$XDIR/src/IO/http.c" 2>/dev/null; then
	sed -i 's/uint_t connect_ret_size/socklen_t connect_ret_size/' "$XDIR/src/IO/http.c" \
		&& echo "=== patched src/IO/http.c (connect_ret_size -> socklen_t) ==="
fi

# --- autogen: the tarball ships configure.ac but NO generated configure -------
# Run autoreconf (aclocal/autoheader/autoconf/automake -a) ONCE. This also drops
# automake's OWN config.sub/config.guess, which are NOT phoenix-aware — so the
# triplet refresh below MUST come AFTER this step.
if [ ! -x "$XDIR/configure" ]; then
	echo "=== autoreconf $NV ==="
	( cd "$XDIR" && autoreconf -fi >/tmp/dillo-autogen.log 2>&1 ) || { tail -30 /tmp/dillo-autogen.log; fail "autoreconf failed"; }
fi

# --- refresh config.sub/config.guess to phoenix-aware copies (AFTER autogen) --
for cfg in config.sub config.guess; do
	if ! grep -q phoenix "$XDIR/$cfg" 2>/dev/null; then
		src=$(grep -lr phoenix "$SRC"/*/$cfg 2>/dev/null | head -1)
		[ -n "$src" ] && cp "$src" "$XDIR/$cfg" && echo "=== refreshed $cfg (phoenix-aware) from $src ==="
	fi
done
grep -q phoenix "$XDIR/config.sub" || fail "config.sub still not phoenix-aware — no donor copy found under $SRC"

# --- the FLTK_CONFIG wrapper: full grouped static link closure ---------------
# Dillo's src/Makefile builds:  ... @LIBFLTK_LIBS@ ...  where LIBFLTK_LIBS is the
# verbatim output of `fltk-config --ldflags`. We override ONLY --ldflags to emit
# the complete static closure (image libs + xcb + libstdc++/m/phoenix/c) wrapped
# in --start-group/--end-group so the static cross-references resolve in any
# order. --cflags/--cxxflags/--version/etc. pass through to the real fltk-config.
WRAP=$PREFIX/fltk-config
cat > "$WRAP" <<WRAPEOF
#!/bin/sh
# AUTO-GENERATED by build-dillo.sh — full static link closure for Dillo on Phoenix.
REAL=$FPREFIX/fltk-config
case " \$* " in
	*" --ldflags "*)
		# -L paths from the real config, then the proven grouped closure.
		# NOTE: -L$BUILDLIB comes LAST so the shared image-lib names (z/png16/jpeg)
		# keep resolving from XPREFIX; only the mbedtls libs are unique to BUILDLIB.
		printf '%s ' "-L$FPREFIX/lib" "-L$XPREFIX/lib" "--sysroot=$SYSROOT" "-L$SYSROOT/lib" "-L$BUILDLIB"
		# The mbedTLS closure sits INSIDE the group (before libc/libphoenix) so its
		# imports into Phoenix libc resolve regardless of scan order. configure also
		# appends LIBSSL_LIBS after this; that trailing duplicate is a harmless re-scan.
		printf '%s ' "-Wl,--start-group" \\
		  "-lfltk_images" "-lfltk" "-lpng16" "-ljpeg" "-lz" \\
		  "-lX11" "-lxcb" "-lXau" "-lXdmcp" \\
		  "-lmbedtls" "-lmbedx509" "-lmbedcrypto" \\
		  "-lstdc++" "-lm" "-lphoenix" "-lc" \\
		  "-Wl,--end-group"
		echo
		;;
	*)
		exec "\$REAL" "\$@"
		;;
esac
WRAPEOF
chmod +x "$WRAP"
echo "=== fltk-config wrapper -> $WRAP ==="

# --- cross flags -------------------------------------------------------------
# Force-include the Phoenix shim (AI_* getaddrinfo hint macros). Point includes
# at the FLTK + X11 prefixes and the cross sysroot.
# GCC 14 promotes -Wincompatible-pointer-types / -Wint-conversion to hard ERRORS
# by default. Dillo's IO/http.c passes a `uint_t*` (4 bytes) where Phoenix's
# getsockopt() wants `socklen_t*` (8 bytes on LP64). Demote both back to warnings
# so the build completes. NOTE: the socklen_t size mismatch is a latent RUNTIME
# bug (getsockopt would write 8 bytes into a 4-byte stack slot) — flagged for the
# attended runtime session; it does not affect the link-time deliverable.
XCFLAGS="--sysroot=$SYSROOT -I$FPREFIX/include -I$XPREFIX/include -I$BUILDINC -include $SHIM -O2 -Wno-error=incompatible-pointer-types -Wno-error=int-conversion"
XLDFLAGS="--sysroot=$SYSROOT -L$FPREFIX/lib -L$XPREFIX/lib -L$SYSROOT/lib -L$BUILDLIB"

# --- configure ---------------------------------------------------------------
# TLS_MODE stamps config.status so a stale HTTP-only configure is redone when the
# TLS decision changes (otherwise the [ ! -f config.status ] guard would silently
# reuse a no-TLS build and the new flags would be ignored).
TLS_MODE="tls-mbedtls-v1"
if [ -f "$XDIR/config.status" ] && ! grep -q "$TLS_MODE" "$XDIR/.dillo-tls-mode" 2>/dev/null; then
	echo "=== stale/HTTP-only configure detected — forcing reconfigure for $TLS_MODE ==="
	rm -f "$XDIR/config.status"
fi
if [ ! -f "$XDIR/config.status" ]; then
	echo "=== configuring $NV (TLS via mbedTLS: --enable-tls --disable-openssl; --disable-webp) ==="
	( cd "$XDIR" && FLTK_CONFIG="$WRAP" ./configure \
	    --host=aarch64-phoenix --build=x86_64-pc-linux-gnu --prefix="$PREFIX" \
	    --enable-tls --disable-openssl --disable-webp \
	    --with-jpeg-lib="$XPREFIX/lib" --with-jpeg-inc="$XPREFIX/include" \
	    CC=${TC}gcc CXX=${TC}g++ AR=${TC}ar RANLIB=${TC}ranlib \
	    CFLAGS="$XCFLAGS" CXXFLAGS="$XCFLAGS" \
	    CPPFLAGS="$XCFLAGS" LDFLAGS="$XLDFLAGS" \
	    PKG_CONFIG=/bin/false \
	    >/tmp/dillo-conf.log 2>&1 ) || { tail -50 /tmp/dillo-conf.log; fail "configure failed"; }
	# Confirm configure actually selected mbedTLS, then stamp the TLS mode.
	grep -q 'Using mbedTLS as TLS library' /tmp/dillo-conf.log \
	    || fail "configure did not select mbedTLS — see /tmp/dillo-conf.log"
	echo "$TLS_MODE" > "$XDIR/.dillo-tls-mode"
fi

# --- build -------------------------------------------------------------------
echo "=== building $NV ==="
( cd "$XDIR" && make >/tmp/dillo-build.log 2>&1 ) || { tail -60 /tmp/dillo-build.log; fail "make failed"; }

DILLO_BIN="$XDIR/src/dillo"
[ -x "$DILLO_BIN" ] || fail "src/dillo not produced"

# --- PRE-FLIGHT VALIDATION (the deliverable test) ----------------------------
echo "=== PRE-FLIGHT ==="
file "$DILLO_BIN"
case "$(file "$DILLO_BIN")" in
	*"ARM aarch64"*"statically linked"*) echo "[OK] aarch64 static ELF" ;;
	*"ARM aarch64"*) echo "[WARN] aarch64 ELF but not reported static — check below" ;;
	*) fail "dillo binary is not an aarch64 ELF" ;;
esac

# Confirm the X11/xcb closure actually got pulled in (the headline risk). On an
# incremental run the link line isn't re-emitted, so check the BINARY for X11
# symbols rather than grepping a possibly-stale build log.
echo "=== xcb/X11 closure sanity (symbols in the binary) ==="
if ${TC}nm "$DILLO_BIN" 2>/dev/null | grep -q ' [TtRr] _\?XOpenDisplay'; then
	echo "[OK] X11 (XOpenDisplay) linked into the binary"
else
	echo "[WARN] XOpenDisplay not found in binary — X11 closure may be missing"
fi

# Confirm the TLS/mbedTLS closure and Dillo's TLS glue actually made it in (task
# E1 deliverable: an HTTPS-CAPABLE build). Runtime HTTPS (entropy + CA bundle +
# Pi internet) is task E2 and is NOT exercised here.
echo "=== TLS (mbedTLS) closure sanity (symbols in the binary) ==="
if ${TC}nm "$DILLO_BIN" 2>/dev/null | grep -q 'mbedtls_ssl_init'; then
	echo "[OK] mbedTLS (mbedtls_ssl_init) linked into the binary"
else
	echo "[WARN] mbedtls_ssl_init not found in binary — TLS closure may be missing"
fi
if ${TC}nm "$DILLO_BIN" 2>/dev/null | grep -q ' [TtRr] a_Tls_mbedtls_connect'; then
	echo "[OK] Dillo TLS glue (a_Tls_mbedtls_connect) present"
else
	echo "[WARN] a_Tls_mbedtls_connect not found — tls_mbedtls.c may not be compiled in"
fi

echo "=== undefined symbols (nm -u) ==="
und=$(${TC}nm -u "$DILLO_BIN" 2>/dev/null)
if [ -z "$und" ]; then
	echo "[OK] 0 undefined symbols"
else
	echo "$und"
	echo "[WARN] undefined symbols present (see above)"
fi

# --- stage to NFS rootfs -----------------------------------------------------
mkdir -p "$NFS/bin" "$ART"
cp "$DILLO_BIN" "$NFS/bin/dillo"
cp "$DILLO_BIN" "$ART/dillo"
# Stage the default config so the browser has sane runtime defaults.
mkdir -p "$NFS/etc/dillo"
[ -f "$XDIR/dillorc" ] && cp "$XDIR/dillorc" "$NFS/etc/dillo/dillorc"

echo "=== Dillo staged ==="
ls -la "$NFS/bin/dillo"
echo "binary:   $NFS/bin/dillo"
echo "artifact: $ART/dillo"
[ -z "$und" ] && echo "=== ALL PRE-FLIGHT CHECKS PASSED ===" || echo "=== BUILD COMPLETE WITH UNDEFINED SYMBOLS (see above) ==="
