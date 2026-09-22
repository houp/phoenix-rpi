#!/bin/bash
# Issue C3: unmounting a USB stick during a write threw the data away and
# returned SUCCESS. This grades the whole correct behaviour, not just "no crash".
#
# Correct outcome (Linux semantics):
#   1. umount DURING the write is REFUSED (busy). The write is untouched.
#   2. the write completes in full -- 200+0 records.
#   3. umount AFTER it finishes succeeds.
#
# Broken outcome (baseline, umountrace-A): step 1 returns rc=0 at t=3 having
# silently torn the filesystem down; dd dies with "Invalid argument" at ~5 MiB.
#
# Grade by the TAG- lines, never by the script's own exit code.
#
# Note: umount by MOUNTPOINT is ENOTSUP in libphoenix; only umount <device>
# works. mount takes positional args, not -t.
set -u
dev="${1:-/dev/umass0}"
mnt="${2:-/mnt/usb}"
mb="${3:-200}"

echo "TAG-start dev=$dev mnt=$mnt mb=$mb"
mkdir -p "$mnt" 2>/dev/null
/bin/mount "$dev" "$mnt" ext2 0 || { echo "TAG-mount FAILED rc=$?"; exit 1; }
echo "TAG-mounted t=$SECONDS"

dd if=/dev/zero of="$mnt/race.bin" bs=1M count="$mb" 2>/tmp/dd.err &
dd_pid=$!
sleep 2

echo "TAG-umount-issued t=$SECONDS"
/bin/umount "$dev"
rc1=$?
echo "TAG-umount-during rc=$rc1 t=$SECONDS   (want NON-ZERO = refused)"

wait "$dd_pid"
dd_rc=$?
echo "TAG-dd-done rc=$dd_rc t=$SECONDS"
echo "TAG-dd-stderr: $(tr '\n' ' ' < /tmp/dd.err)"

# Did all the data land?
sz=$(/bin/ls -l "$mnt/race.bin" 2>/dev/null | /usr/bin/awk '{print $5}')
echo "TAG-size bytes=${sz:-none} want=$((mb * 1048576))"

echo "TAG-umount-retry-issued t=$SECONDS"
/bin/umount "$dev"
rc2=$?
echo "TAG-umount-after rc=$rc2 t=$SECONDS   (want 0 = succeeded)"
echo "TAG-done"
