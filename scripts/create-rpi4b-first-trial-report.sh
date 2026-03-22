#!/usr/bin/env bash

set -euo pipefail

image_path="${RPI4B_SDIMG_PATH:-/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img}"
report_dir="${RPI4B_REPORT_DIR:-/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-reports}"
timestamp="$(date +%Y%m%d-%H%M%S)"
report_path="${RPI4B_REPORT_PATH:-$report_dir/pi4-first-trial-$timestamp.md}"

if [ ! -f "$image_path" ]; then
	printf 'missing image: %s\n' "$image_path" >&2
	exit 1
fi

image_sha256="${RPI4B_SDIMG_SHA256:-$(shasum -a 256 "$image_path" | awk '{print $1}')}"

mkdir -p "$report_dir"

cat > "$report_path" <<EOF
# Pi 4 First Hardware Trial Report

- Image: $image_path
- SHA-256: $image_sha256

## Hardware

- Board revision:
- Display:
- Keyboard:
- Ethernet attached: yes/no

## Observed Class

- firmware-load / early-boot / runtime-no-input / runtime-shell / reboot-loop / unknown

## Timing

- Power-on time observed:

## HDMI Result

- no signal / brief flash / stable picture
- early panel seen: yes/no
- black text console seen: yes/no
- prompt seen: yes/no

## Keyboard Result

- no visible effect / partial / full
- keys tried:

## Command Results

- help:
- ps:
- ls /:

## LED / Reboot Behavior

-

## Additional Notes

-
EOF

printf 'Created: %s\n' "$report_path"
