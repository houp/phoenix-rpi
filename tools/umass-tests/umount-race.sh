#!/bin/bash
# Does umount WAIT for in-flight filesystem requests, or free the context under them?
#
# Test for devices branch agent/umass-umount-drain. It must have I/O ACTUALLY IN
# FLIGHT when the umount lands -- "mount, copy a file, umount" passes on the
# broken code too, because nothing is inside libext2_handler by then.
#
#   broken: umount returns almost at once (t ~= 2s). The dd then runs against a
#           freed context -- look for Exception / Data Abort, or a wedge.
#   fixed:  umount BLOCKS until the dd finishes, then returns 0. The WAITING is
#           the observable: TAG-umount-returned t= should be close to TAG-dd-done.
#
# Note: umount by MOUNTPOINT is ENOTSUP in libphoenix (sys/mount.c:154); only
# umount <device> works. mount takes positional args here, not -t.
set -u
dev="${1:-/dev/umass0}"
mnt="${2:-/mnt/usb}"
mb="${3:-200}"

echo "TAG-start dev=$dev mnt=$mnt mb=$mb"
mkdir -p "$mnt" 2>/dev/null
/bin/mount "$dev" "$mnt" ext2 0 || { echo "TAG-mount FAILED rc=$?"; exit 1; }
echo "TAG-mounted t=$SECONDS"

dd if=/dev/zero of="$mnt/race.bin" bs=1M count="$mb" &
dd_pid=$!
sleep 2
echo "TAG-umount-issued t=$SECONDS"
/bin/umount "$dev"
rc=$?
echo "TAG-umount-returned rc=$rc t=$SECONDS"

wait "$dd_pid" 2>/dev/null
echo "TAG-dd-done t=$SECONDS"
echo "TAG-done"
