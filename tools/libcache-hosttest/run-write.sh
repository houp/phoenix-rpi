#!/usr/bin/env bash
# A/B the ranged write-back on the host: default (whole-line flush) vs 512 B.
# The ratio between the two AMPLIFICATION numbers is the same quantity the
# 21.02x -> 1.00x hardware result measured.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
out="$here/libcache-write-shapes"
gcc -O1 -g -fsanitize=address,undefined -DEOK=0 \
    -I"$here" -I"$root/sources/phoenix-rtos-corelibs/libcache" \
    "$here/libcache-write-shapes.c" \
    "$root/sources/phoenix-rtos-corelibs/libcache/cache.c" \
    "$root/sources/libphoenix/sys/list.c" -o "$out"
for g in 0 512; do
  echo "--- granularity=$g"
  ASAN_OPTIONS=detect_leaks=0 "$out" "$g"
  echo
done
