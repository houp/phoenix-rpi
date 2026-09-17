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
# Exit 0 = every binary is at least as new as libphoenix.a; 1 = stale ones exist;
# 2 = the census could not run (no libphoenix.a, no root, no binaries found).
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

# Coverage: every directory on the image that holds an executable, depth 1.
# /sbin and /usr/sbin were NOT scanned before 2026-09-17 although the banner
# said "every binary" — that silently exempted init, getty, syslogd, hwclock,
# nfs, dropbear and lighttpd from the one check that catches an ABI break.
scan_dirs=()
for d in bin usr/bin sbin usr/sbin; do
	[ -d "$root/$d" ] && scan_dirs+=("$root/$d")
done
scan_desc="$(printf '%s ' "${scan_dirs[@]#"$root/"}")"
if [ "${#scan_dirs[@]}" -eq 0 ]; then
	echo "check-stale-binaries: none of bin/ usr/bin/ sbin/ usr/sbin/ exist under $root" >&2
	exit 2
fi
echo "scanning : $scan_desc(depth 1)"

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
done < <(find "${scan_dirs[@]}" -maxdepth 1 -type f 2>/dev/null)

# ⚠ A census that scanned nothing is not a clean census. Before 2026-09-17 only
# $root/bin and $root/usr/bin were scanned, and neither was checked for
# existence: a root with a different layout produced total=0, stale=0 and
# "RESULT: PASS -- every binary is at least as new as libphoenix.a".
if [ "$total" -eq 0 ]; then
	echo "RESULT: FAIL — scanned $scan_desc and found NO binaries at all." >&2
	echo "That is a broken census, not a clean tree. Check the root layout." >&2
	exit 2
fi

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
	echo "RESULT: FAIL — $stale of $total binaries predate libphoenix.a."
	echo "A timestamp proves only that they were NOT relinked against it. If that"
	echo "rebuild changed a syscall signature they carry the OLD ABI and can fail"
	echo "nondeterministically; if it changed nothing they are merely older. The"
	echo "census cannot tell the two apart — so after a merge touching libphoenix"
	echo "or a syscall signature, do NOT trust a showcase gate or cut an image"
	echo "until these are rebuilt. (A plain core rebuild bumps libphoenix.a's"
	echo "mtime too, so this list reads long on a perfectly healthy tree.)"
	exit 1
fi
if [ "$probes" -gt 0 ]; then
	echo "RESULT: PASS — the shipping surface ($total binaries in $scan_desc) is fully rebuilt (the $probes probe(s) above are not)."
else
	echo "RESULT: PASS — all $total binaries in $scan_desc are at least as new as libphoenix.a."
fi
exit 0
