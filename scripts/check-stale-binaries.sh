#!/usr/bin/env bash
#
# check-stale-binaries.sh — after a merge that changes libphoenix, find every
# binary that was NOT rebuilt against it.
#
# WHY THIS EXISTS (2026-09-16). The upstream sweep changed phMutexLock/phCondWait
# from ONE stack argument to THREE. A binary linked against the old libphoenix
# passes only the handle, so the kernel reads uninitialised stack as the clock id
# and pthread_mutex_lock() returns EINVAL on a correctly-initialised mutex.
#
# The failure is NONDETERMINISTIC per binary -- the deciding value is garbage --
# so apps can PASS WHILE STALE. On that merge QuakeSpasm and vkQuake passed while
# stale, the six-app gate went 6/6, 1050 libc tests went green, and 189 of 333
# binaries were still on the old ABI. The green gate was measuring only the
# subset that had been rebuilt.
#
# A timestamp census is the check that catches this, and it is cheap. Run it after
# ANY merge that touches libphoenix or a syscall signature, BEFORE believing a
# gate. See docs/misc/2026-09-16-upstream-mutex-abi-break.md.
#
#   scripts/check-stale-binaries.sh [root]      # default: the live fsid=0 export
#
# Exit 0 = every binary is at least as new as libphoenix.a; 1 = stale ones exist.
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
root="${1:-/srv/phoenix-rpi4-nfs-gcc16}"
libc="$repo/.buildroot/_build/aarch64a72-generic-rpi4b/lib/libphoenix.a"

[ -e "$libc" ] || { echo "check-stale-binaries: no libphoenix.a at $libc" >&2; exit 2; }
[ -d "$root" ] || { echo "check-stale-binaries: no such root: $root" >&2; exit 2; }

ref="$(date -r "$libc" '+%Y-%m-%d %H:%M:%S')"
echo "reference: libphoenix.a built $ref"
echo "root     : $root"

# Ad-hoc DIAGNOSTIC PROBES, built by hand during a debug session and not by the
# standard build flow, so a normal rebuild never refreshes them. They are not in
# the boot, showcase or reel path, so their staleness cannot affect a gate or an
# image -- they are reported as a NOTE rather than a failure.
#
# ⚠ This list exists so the check can reach PASS on a healthy tree. A check that
# can never pass is a check people stop reading, which would defeat the purpose.
# It is NOT permission to ignore them: anything here is on the OLD syscall ABI and
# must be rebuilt before it is trusted, or it will fail the same way hevc-play did
# (every decode rc=-5) -- and that cost an hour of blaming the wrong thing.
DIAGNOSTIC_PROBES="dfprobe dfprobe2 memprobe cxxprobe cxxprobeold cxxprobebr \
rpi4-ipcprobe boshare-probe udprtt fileperf gl-fbo-orient gl-bo-import xresizer"

is_probe() {
	case " $DIAGNOSTIC_PROBES " in *" $1 "*) return 0 ;; *) return 1 ;; esac
}

total=0; stale=0; stale_list=""; probes=0; probe_list=""
while IFS= read -r f; do
	total=$((total + 1))
	if [ "$f" -ot "$libc" ]; then
		b="$(basename "$f")"
		if is_probe "$b"; then
			probes=$((probes + 1)); probe_list="$probe_list $b"
		else
			stale=$((stale + 1)); stale_list="$stale_list $b"
		fi
	fi
done < <(find "$root/bin" "$root/usr/bin" -maxdepth 1 -type f 2>/dev/null)

echo "binaries : $total"
echo "stale    : $stale  (shipping surface)"
if [ "$probes" -gt 0 ]; then
	# shellcheck disable=SC2086
	echo "probes   : $probes stale diagnostic tool(s), NOT in the boot/showcase/reel path:"
	echo "            $(echo $probe_list | tr -s ' ')"
	echo "            rebuild any of these before trusting its output."
fi
if [ "$stale" -gt 0 ]; then
	# shellcheck disable=SC2086
	printf '  %s\n' $stale_list | head -30
	[ "$stale" -gt 30 ] && echo "  … and $((stale - 30)) more"
	echo
	echo "RESULT: FAIL — $stale binary(ies) predate libphoenix.a."
	echo "They still carry the OLD syscall ABI and can fail nondeterministically."
	echo "Do NOT trust a showcase gate or cut an image until these are rebuilt."
	exit 1
fi
if [ "$probes" -gt 0 ]; then
	echo "RESULT: PASS — the shipping surface is fully rebuilt (the $probes probe(s) above are not)."
else
	echo "RESULT: PASS — every binary is at least as new as libphoenix.a."
fi
exit 0
