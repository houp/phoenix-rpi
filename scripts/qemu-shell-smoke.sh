#!/usr/bin/env bash

set -euo pipefail


usage() {
	printf 'Usage: %s <generic|rpi4b>\n' "${0##*/}" >&2
}


if [ "$#" -ne 1 ]; then
	usage
	exit 2
fi

target="$1"
vm="${PHOENIX_VM:-phoenix-dev}"

case "$target" in
	generic)
		limactl shell -y "$vm" -- /bin/bash -lc 'cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy && expect -c "set timeout 35; log_user 1; spawn /usr/bin/qemu-system-aarch64 -machine virt,secure=on,gic-version=2 -cpu cortex-a53 -smp 1 -m 1G -nographic -monitor none -kernel _boot/aarch64a53-generic-qemu/plo.elf -device loader,file=_boot/aarch64a53-generic-qemu/loader.disk,addr=0x48000000,force-raw=on; expect \"(psh)%\"; sleep 1; send \"help\r\"; expect \"Available commands:\"; expect \"(psh)%\"; send \003; expect eof" > /tmp/generic-shell-smoke.log 2>&1 && grep -En "Available commands:|\(psh\)%|help" /tmp/generic-shell-smoke.log | tail -n 20 && printf "Log: %s\n" /tmp/generic-shell-smoke.log'
		;;
	rpi4b)
		limactl shell -y "$vm" -- /bin/bash -lc 'cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy && expect -c "set timeout 90; log_user 1; spawn /home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none -kernel _boot/aarch64a72-generic-rpi4b/plo.elf -device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x08000000,force-raw=on; expect \"(psh)%\"; sleep 1; send \"help\r\"; expect \"Available commands:\"; expect \"(psh)%\"; send \003; expect eof" > /tmp/pi4-shell-smoke.log 2>&1 && grep -En "Available commands:|\(psh\)%|help" /tmp/pi4-shell-smoke.log | tail -n 20 && printf "Log: %s\n" /tmp/pi4-shell-smoke.log'
		;;
	*)
		usage
		exit 2
		;;
esac
