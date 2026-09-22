#!/bin/bash
# Does umount wait for in-flight filesystem requests, or free the context under them?
#
# This is the test for devices branch agent/umass-umount-drain. It has to have
# I/O ACTUALLY IN FLIGHT when umount arrives -- "mount, copy a file, umount"
# passes on the broken code too, because nothing is in the handler by then.
#
#   old code: umount returns ~immediately, and the dd either faults, wedges, or
#             completes against freed memory. Look for Exception / Data Abort.
#   new code: umount BLOCKS until the dd finishes, then returns 0. The WAITING
#             is the observable -- expect umount's elapsed time to be close to
#             dd's, not close to zero.
#
# usage: /bin/bash /root/umount-race.sh [/dev/umass0] [/mnt/usb]
set -u
dev="${1:-/dev/umass0}"
mnt="${2:-/mnt/usb}"

echo "TAG-start dev=$dev mnt=$mnt"
mkdir -p "$mnt" 2>/dev/null
/bin/mount -t ext2 "$dev" "$mnt" || { echo "TAG-mount FAILED"; exit 1; }
echo "TAG-mounted"

# Enough work that the handler is certainly still busy when umount lands.
dd if=/dev/zero of="$mnt/race.bin" bs=1M count=200 &
dd_pid=$!
sleep 2
echo "TAG-umount-issued t=$SECONDS"
/bin/umount "$mnt"
rc=$?
echo "TAG-umount-returned rc=$rc t=$SECONDS"

wait "$dd_pid" 2>/dev/null
echo "TAG-dd-done t=$SECONDS"
echo "TAG-verdict umount waited for $SECONDS s total; near 2 s means it did NOT drain"
