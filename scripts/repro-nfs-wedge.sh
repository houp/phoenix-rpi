#!/usr/bin/env bash
#
# repro-nfs-wedge.sh — reproduce the nfs-fs wedge (and with it `premain-hang`)
# ON DEMAND, instead of waiting for a ~13% random event.
#
# What it does: boots the Pi, waits for the psh prompt, then restarts the HOST's
# nfsd. That drops our NFSv4 client state (NFS4ERR_EXPIRED / ERANGE on the Pi),
# which is the condition the spontaneous failures were traced to. nfs-fs then
# stops servicing its message loop and every later launch hangs, because the
# server is single-threaded and the fs-path IPC has no timeout.
#
# Observed 2/2 on 2026-09-15: a WEDGE report with a full ring of lookups, and the
# app launch never reaching Host_Init. Note NO "reclaimed NFSv4 client state"
# line appears -- the recovery path does not complete.
#
# Build the diagnostic first, or there is nothing to read:
#   FS_DIAG='-DNFS_MSG_TICK' LIBC_STARTUP_TRACE=min \
#       ./scripts/rebuild-rpi4b-fast.sh --scope core --variant nfsroot
#   ./scripts/sync-netboot-tree.sh
#
# Usage: ./scripts/repro-nfs-wedge.sh [label] [seconds-after-prompt]
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:-nfswedge}"
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
printf 'repro-nfs-wedge: restarting host nfsd at %s\n' "$(date +%H:%M:%S)"
sudo systemctl restart nfs-server

wait "$cycle" 2>/dev/null || true
uart=$(ls -t "${repo}/artifacts/rpi4b-uart/"*"${label}".log 2>/dev/null | head -1)
printf 'repro-nfs-wedge: UART log %s\n' "$uart"
