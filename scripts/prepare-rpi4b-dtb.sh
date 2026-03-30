#!/usr/bin/env bash

set -euo pipefail

vm="${PHOENIX_VM:-phoenix-dev}"
linux_tree="${RPI4B_LINUX_TREE:-/Users/witoldbolt/phoenix-rpi/external/raspberrypi-linux}"
source_dts="${RPI4B_SOURCE_DTS:-$linux_tree/arch/arm/boot/dts/broadcom/bcm2711-rpi-4-b.dts}"
out_dir="${RPI4B_DTB_DIR:-/tmp/rpi4b-dtb}"
out_dtb="${RPI4B_DTB_PATH:-$out_dir/bcm2711-rpi-4-b.dtb}"

limactl shell -y "$vm" -- bash -lc "
set -euo pipefail

linux_tree='$linux_tree'
source_dts='$source_dts'
out_dir='$out_dir'
out_dtb='$out_dtb'

if [ ! -f \"\$source_dts\" ]; then
	printf 'missing Pi 4 DTS source: %s\n' \"\$source_dts\" >&2
	exit 1
fi

mkdir -p \"\$out_dir\"
cd \"\$linux_tree\"

cpp -nostdinc \
	-I arch/arm/boot/dts \
	-I arch/arm/boot/dts/broadcom \
	-I include \
	-undef \
	-x assembler-with-cpp \
	\"\$source_dts\" | dtc -I dts -O dtb -o \"\$out_dtb\"

printf 'Prepared: %s\n' \"\$out_dtb\"
wc -c < \"\$out_dtb\"
"
