#!/usr/bin/env bash
#
# test-nfs-recovery.sh — the regression gate for `premain-hang`.
#
# Boots the Pi on an NFS root, waits for the psh prompt, then restarts the HOST's
# nfsd under it. That drops our NFSv4 client state and forces libnfs to
# reconnect, which is exactly what used to deadlock the single-threaded nfs-fs:
# socket() resolved /dev/netsocket by PATH, and a path lookup is answered by the
# filesystem that owns "/" -- which nfs-fs IS. Every process then blocked on its
# first filesystem request, so a launch hung while opening its own binary.
#
# Fixed 2026-09-15 by caching the resolved oid (phoenix-rtos-kernel 7c021702,
# libphoenix 47dde32; libnfs patch 04 adds a sync-call deadline as defence in
# depth, filesystems 427aabd reclaims on it). This script grades that fix rather
# than leaving it to be eyeballed in a log.
#
# PASS requires, after the restart:
#   1. nfs-fs notices                -> "reclaiming client state"
#   2. the reclaim COMPLETES         -> "reclaimed NFSv4 client state"
#   3. a fresh process still launches -> quakespasm reaches "Host_Init"
# and no WEDGE report and no fault anywhere in the log.
#
# Run it on a CLEAN build (no FS_DIAG / LIBC_STARTUP_TRACE): the markers above
# are ordinary server messages, not diagnostics.
#
#   ./scripts/rebuild-rpi4b-fast.sh --scope core && ./scripts/sync-netboot-tree.sh
#   ./scripts/test-nfs-recovery.sh
#
# Exits 0 on PASS, 1 on FAIL. Card must be OUT of the Pi (netboot).
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:-nfsrecovery}"
delay="${2:-12}"
log="${repo}/artifacts/${label}-cycle.log"

cd "$repo"
nohup ./scripts/test-cycle-psh-interact.sh --label "$label" --wait-secs 220 \
	--inter-cmd-secs 8 --idle-secs 90 --max-cmd-secs 260 \
	-- "/bin/test-libc-unix-socket -v" "/usr/bin/quakespasm -loadbench" > "$log" 2>&1 &
cycle=$!

# Wait for the prompt, then let the first command get going before pulling the rug.
for _ in $(seq 1 26); do
	grep -qa 'marker seen' "$log" 2>/dev/null && break
	sleep 10
done
sleep "$delay"
# Where the UART log stood WHEN the rug was pulled. Without this, criterion 3
# below ("a fresh process still launches after the restart") counted Host_Init
# anywhere in the log — including the launch that happened BEFORE the restart,
# which proves nothing about recovery. Nothing writes a restart marker into the
# UART stream, so the line count is the only available boundary.
uart_live=$(ls -t "${repo}/artifacts/rpi4b-uart/"*"${label}".log 2>/dev/null | head -1)
mark_line=0
[ -n "$uart_live" ] && mark_line=$(wc -l < "$uart_live" | tr -d ' ')
printf 'test-nfs-recovery: restarting host nfsd at %s (UART log at line %s)\n' \
	"$(date +%H:%M:%S)" "$mark_line"
sudo systemctl restart nfs-server

wait "$cycle" 2>/dev/null || true
uart=$(ls -t "${repo}/artifacts/rpi4b-uart/"*"${label}".log 2>/dev/null | head -1)
printf 'test-nfs-recovery: UART log %s\n' "$uart"

if [ -z "$uart" ]; then
	printf 'test-nfs-recovery: FAIL — no UART log (cycle never ran)\n'
	exit 1
fi

# `grep -c` prints 0 and EXITS 1 with no match, so `|| true`, never `|| echo 0`.
noticed=$(grep -ac 'reclaiming client state' "$uart" || true)
reclaimed=$(grep -ac 'reclaimed NFSv4 client state' "$uart" || true)
launched=$(tail -n "+$((mark_line + 1))" "$uart" | grep -ac 'Host_Init' || true)
wedged=$(grep -ac 'nfs-fs: WEDGE,' "$uart" || true)
# Ask the script that owns the fault set rather than carrying a narrower copy
# (this one had no allocator patterns and no truncated-message class).
faults=$("${repo}/scripts/uart-summary.sh" "$uart" 2>/dev/null \
	| sed -n 's/^fault_pattern_matches: //p' | head -1)
[ -n "$faults" ] || faults=0

printf '  noticed=%s reclaimed=%s launched(after restart)=%s wedged=%s faults=%s\n' \
	"$noticed" "$reclaimed" "$launched" "$wedged" "$faults"

rc=0
[ "$noticed"   -ge 1 ] || { printf '  FAIL: nfs-fs never noticed the dropped state\n'; rc=1; }
[ "$reclaimed" -ge 1 ] || { printf '  FAIL: reclaim never completed (the server is stuck)\n'; rc=1; }
[ "$launched"  -ge 1 ] || { printf '  FAIL: no process launched after the restart (premain-hang)\n'; rc=1; }
[ "$wedged"    -eq 0 ] || { printf '  FAIL: nfs-fs reported a WEDGE\n'; rc=1; }
[ "$faults"    -eq 0 ] || { printf '  FAIL: %s fault line(s) in the log\n' "$faults"; rc=1; }

if [ "$rc" -eq 0 ]; then
	printf 'test-nfs-recovery: PASS\n'
else
	printf 'test-nfs-recovery: FAIL\n'
fi
exit "$rc"
