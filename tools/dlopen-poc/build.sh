#!/usr/bin/env bash
#
# Build the Phase-A dlopen PoC (T-DYNLINK): a static host that loads a -fPIC
# ET_DYN plugin at runtime via the minidl loader. Stages both into the netboot
# NFS root export so a netboot cycle can run them.
#
# Mirrors tools/bt-probe/build.sh (no phoenix-rtos-build machinery).
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$HERE/../.." && pwd)"
GCC="${GCC:-$REPO_ROOT/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-gcc}"
NM="${NM:-$REPO_ROOT/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-nm}"
READELF="${READELF:-$REPO_ROOT/.toolchain/aarch64-phoenix/bin/aarch64-phoenix-readelf}"
NFSROOT="${NFSROOT:-/srv/phoenix-rpi4-nfs}"

HOST="$HERE/dlopen-poc"
PLUGIN="$HERE/plugin.so"

echo "dlopen-poc: building plugin (-shared -fPIC, libc left undefined, sysv hash)"
# -nostdlib: keep printf/host_add UNDEFINED (resolved from the host at load time,
#            avoiding a second libc instance). --hash-style=sysv: emit DT_HASH so
#            the loader can size .dynsym via nchain.
"$GCC" -shared -fPIC -nostdlib -Wl,--hash-style=sysv -O2 -Wall \
	-o "$PLUGIN" "$HERE/plugin.c"

echo "dlopen-poc: plugin dynamic relocations (want RELATIVE/GLOB_DAT/JUMP_SLOT only):"
"$READELF" -r "$PLUGIN" 2>/dev/null | grep -E 'R_AARCH64' | awk '{print "  " $3}' | sort | uniq -c || true
echo "dlopen-poc: plugin undefined syms (want: printf, host_add):"
"$NM" -u "$PLUGIN" 2>/dev/null | awk '{print "  " $2}' || true

echo "dlopen-poc: building host (static ET_EXEC)"
"$GCC" -O2 -Wall -Wextra -std=gnu11 -o "$HOST" "$HERE/main.c" "$HERE/minidl.c"

echo "dlopen-poc: host undefined-symbol check (expect none):"
undef=$("$NM" -u "$HOST" 2>/dev/null | grep -vE 'GLIBC|^\s*$' | wc -l || true)
echo "  $undef undefined symbols."

file "$HOST" "$PLUGIN"

if [ -d "$NFSROOT/usr/bin" ]; then
	echo "dlopen-poc: staging into $NFSROOT/usr/bin"
	sudo cp "$HOST" "$NFSROOT/usr/bin/dlopen-poc"
	sudo cp "$PLUGIN" "$NFSROOT/usr/bin/plugin.so"
	echo "dlopen-poc: staged. Run on the Pi via psh: /usr/bin/dlopen-poc"
else
	echo "dlopen-poc: NFS root $NFSROOT not present; skipped staging."
fi
echo "dlopen-poc: done."
