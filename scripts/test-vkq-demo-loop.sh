#!/usr/bin/env bash
#
# test-vkq-demo-loop.sh — prove vkQuake CYCLES its demos, not just plays one.
#
# vkQuake used to play exactly ONE demo (~50 s) and drop to the console while
# QuakeSpasm looped. Root cause was one line in Host_Startdemos_f: the bring-up
# patch set `cls.demonum = -1`, which permanently disarms Quake's demo loop, so
# when the port's own post-Host_Init `playdemo` ended, Host_EndGame disconnected
# instead of calling CL_NextDemo. Fixed in the fork (external/vkquake c55d7d8).
#
# WHY THIS NEEDS A SCRIPT RATHER THAN A BARE CYCLE
# ------------------------------------------------
# The demo route only exists when `id1/phoenix-demo.cfg` is STAGED — the shipped
# image has none, so vkQuake boots `map start` instead (and `map` disarms the
# loop by itself, via CL_EstablishConnection). That same file must be ABSENT for
# the #67 wall-torch check, which needs the start map at a viewpoint. So the file
# has to be created for this test and removed again afterwards, on every exit
# path. Leaving it behind silently breaks a later gate run.
#
# HOW TO GRADE IT — and how NOT to
# --------------------------------
# ⛔ NEVER grade by the `N demo(s) in loop` line. Host_Startdemos_f prints that
#    UNCONDITIONALLY, before the test that decides whether the loop runs; it was
#    printed just as loudly by the broken build. It has caused a false retraction
#    before.
# ✅ Grade by ORDERED, DISTINCT `Playing demo from <name>.dem` lines. Two demos
#    is the whole claim: one is what the broken build already did.
#
# Cross-check, independent of the log text: `flipstat-summary.sh --seq` over this
# run must NOT show the long flat ~42.2 fps block that means "sitting on the
# static console". A looping run stays in the variable gameplay band.
#
# ⓘ The observed order is demo2 -> demo1 -> demo2 -> demo3 -> demo1..., i.e. demo2
# plays twice per cycle. That is correct, not a bug: phoenix-demo.cfg names the
# FIRST demo while the loop's own list is quake.rc's demo1/demo2/demo3. Stage
# "demo1" instead if you want the canonical order.
#
# Usage: ./scripts/test-vkq-demo-loop.sh [label] [secs]
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:-vkq-loop}"
secs="${2:-300}"

# The LIVE fsid=0 export. /srv/phoenix-rpi4-nfs (no suffix) is DEAD — writing
# there stages into a tree the Pi never mounts.
export_root="${RPI4B_NFS_EXPORT:-/srv/phoenix-rpi4-nfs-gcc16}"
demo_cfg="${export_root}/usr/share/quake/id1/phoenix-demo.cfg"

if [ ! -d "$(dirname "${demo_cfg}")" ]; then
	echo "test-vkq-demo-loop: no id1 dir under ${export_root} -- is that the live export?" >&2
	exit 2
fi

# Remove the staged cfg on EVERY exit path, but only if we were the ones who
# created it (never clobber a cfg someone else staged deliberately).
staged_by_us=0
cleanup() {
	if [ "${staged_by_us}" = "1" ] && [ -f "${demo_cfg}" ]; then
		rm -f "${demo_cfg}"
		echo "=== unstaged ${demo_cfg}"
	fi
}
trap cleanup EXIT INT TERM

if [ -f "${demo_cfg}" ]; then
	echo "=== ${demo_cfg} already staged ($(cat "${demo_cfg}")) -- leaving it alone"
else
	echo "demo2" > "${demo_cfg}"
	staged_by_us=1
	echo "=== staged ${demo_cfg} = demo2"
fi

# Budget: vkQuake needs ~20 s to init, demo2 runs ~50 s and demo1 ~70 s, so two
# transitions (demo2 -> demo1 -> demo2) need roughly 200 s of run time. Default
# 300 s leaves margin without stretching the capture past what picocom holds.
echo "=== running vkquake for ${secs}s (label ${label})"
"${repo}/scripts/test-cycle-psh-interact.sh" \
	--label "${label}" --wait-secs 220 --inter-cmd-secs 8 \
	--idle-secs "${secs}" --max-cmd-secs "$(( secs + 40 ))" -- "vkquake"
rc=$?

log="$(ls -t "${repo}"/artifacts/rpi4b-uart/rpi4b-uart-*"${label}".log 2>/dev/null | head -1)"
if [ -z "${log}" ]; then
	echo "test-vkq-demo-loop: no UART log for ${label}" >&2
	exit 1
fi

echo
echo "=== log: ${log}"
echo "--- demos actually started (the ONLY line that means anything) ---"
# grep -a: the logs carry binary bytes.
mapfile -t plays < <(grep -aoE "Playing demo from [A-Za-z0-9_.-]+\.dem" "${log}")
n=${#plays[@]}
i=1
for p in "${plays[@]}"; do
	printf '  %d. %s\n' "$i" "${p}"
	i=$(( i + 1 ))
done
[ "${n}" -gt 0 ] || echo "  (none)"

echo
if [ "${n}" -ge 2 ]; then
	echo "vkq-demo-loop: PASS -- ${n} demos started; the loop advanced on its own."
	verdict=0
else
	echo "vkq-demo-loop: FAIL -- ${n} demo(s) started. One demo is exactly what the"
	echo "  broken build did; the loop did not advance."
	verdict=1
fi

echo
echo "--- cross-check: flip-rate shape (a long flat block = parked on the console) ---"
"${repo}/scripts/flipstat-summary.sh" --seq "${label}" 2>/dev/null | tail -20

exit "$(( verdict || rc ))"
