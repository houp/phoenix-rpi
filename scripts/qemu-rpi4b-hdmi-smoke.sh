#!/usr/bin/env bash

set -euo pipefail


vm="${PHOENIX_VM:-phoenix-dev}"


limactl shell -y "$vm" -- /bin/bash -lc '
set -euo pipefail

buildroot=/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
qemu=/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64
tmpbase=/tmp/pi4-hdmi-smoke-$$
monitor_sock="${tmpbase}.sock"
serial_log="${tmpbase}.log"
ppm="${tmpbase}.ppm"
monitor_out="${tmpbase}.out"
qemu_out="${tmpbase}.qemu"

cd "$buildroot"
rm -f "$monitor_sock" "$serial_log" "$ppm" "$monitor_out" "$qemu_out"

"$qemu" \
	-M raspi4b -cpu cortex-a72 -smp 4 -m 2G \
	-display vnc=:4 \
	-serial "file:${serial_log}" \
	-monitor "unix:${monitor_sock},server,nowait" \
	-kernel _boot/aarch64a72-generic-rpi4b/plo.elf \
	-device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on \
	>"${qemu_out}" 2>&1 &
qpid=$!

for _ in $(seq 1 40); do
	[ -S "$monitor_sock" ] && break
	sleep 0.25
done

if [ ! -S "$monitor_sock" ]; then
	echo "QEMU monitor socket did not appear: ${monitor_sock}" >&2
	wait "$qpid" || true
	exit 1
fi

sleep 8

{
	printf "screendump %s\n" "$ppm"
	printf "quit\n"
} | socat - "UNIX-CONNECT:${monitor_sock}" >"$monitor_out"

wait "$qpid" || true

python3 - "$ppm" <<'"'"'PY'"'"'
import sys
from pathlib import Path

ppm = Path(sys.argv[1])

if not ppm.exists():
    raise SystemExit(f"Missing framebuffer dump: {ppm}")

with ppm.open("rb") as f:
    magic = f.readline().strip()
    dims = f.readline().strip()
    maxv = f.readline().strip()
    data = f.read()

if magic != b"P6":
    raise SystemExit(f"Unexpected PPM magic: {magic!r}")

width, height = map(int, dims.split())
if (width, height) != (1024, 768):
    raise SystemExit(f"Unexpected framebuffer size: {(width, height)}")

def px(x, y):
    i = (y * width + x) * 3
    return tuple(data[i:i + 3])

for pt in ((20, 20), (48, 48), (112, 48), (176, 48)):
    got = px(*pt)
    if got != (0, 0, 0):
        raise SystemExit(f"Expected black background at {pt}, got {got}")

white = black = bg = other = 0
for y in range(96, 128):
    for x in range(0, 240):
        p = px(x, y)
        if p == (255, 255, 255):
            white += 1
        elif p == (0, 0, 0):
            black += 1
        elif p == (160, 96, 48):
            bg += 1
        else:
            other += 1

if white < 1000:
    raise SystemExit(f"Too few white text pixels in banner row: {white}")

if black < 5000:
    raise SystemExit(f"Too few black background pixels in banner row: {black}")

if bg != 0:
    raise SystemExit(f"Unexpected pre-console background pixels in banner row: {bg}")

if other != 0:
    raise SystemExit(f"Unexpected non-binary text colors in banner row: {other}")

print("Pi 4 HDMI smoke passed")
print(f"Framebuffer: {width}x{height}")
print(f"text row white={white} black={black}")
PY

printf "Serial log: %s\n" "$serial_log"
'
