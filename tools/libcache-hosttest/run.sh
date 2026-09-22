#!/usr/bin/env bash
# Host-side unit test for libcache's read path, built with ASan+UBSan.
#
# Why this exists: a 16 MiB contiguous ext2 read at 1 KiB blocks faulted the
# bcm2711-emmc driver (2026-09-22). libcache was the prime suspect, since that
# read reaches it as a single 16 MiB cache_read(). This harness reproduces the
# exact shape on the host and shows the assembly is CORRECT -- which is what
# moved the suspicion off libcache and onto how long the driver lock is held.
#
# It also prints the largest single device read, which demonstrates that
# cache_read() already chunks device I/O to one lineSize per fetch.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
out="$here/libcache-read-shapes"
gcc -O1 -g -fsanitize=address,undefined -DEOK=0 \
    -I"$here" -I"$root/sources/phoenix-rtos-corelibs/libcache" \
    "$here/libcache-read-shapes.c" \
    "$root/sources/phoenix-rtos-corelibs/libcache/cache.c" \
    "$root/sources/libphoenix/sys/list.c" \
    -o "$out"
# No cache_deinit() in the harness, so leaks are expected and not the subject.
ASAN_OPTIONS=detect_leaks=0 "$out"
