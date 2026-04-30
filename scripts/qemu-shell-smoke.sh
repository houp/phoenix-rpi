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
		timeout_secs=35
		log_path="/tmp/generic-shell-smoke.log"
		qemu_cmd="/usr/bin/qemu-system-aarch64 -machine virt,secure=on,gic-version=2 -cpu cortex-a53 -smp 1 -m 1G -nographic -monitor none -kernel _boot/aarch64a53-generic-qemu/plo.elf -device loader,file=_boot/aarch64a53-generic-qemu/loader.disk,addr=0x48000000,force-raw=on"
		;;
	rpi4b)
		timeout_secs=90
		log_path="/tmp/pi4-shell-smoke.log"
		qemu_cmd="/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none -kernel _boot/aarch64a72-generic-rpi4b/plo.elf -device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x08000000,force-raw=on"
		;;
	*)
		usage
		exit 2
		;;
esac

remote_cmd=$(cat <<EOF
cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
expect -c '
set timeout $timeout_secs
log_user 1
proc must_expect {pattern} {
	expect {
		-re \$pattern {}
		timeout { exit 124 }
		eof { exit 125 }
	}
}
spawn $qemu_cmd
must_expect {\\(psh\\)%}
sleep 1
send "help\\r"
must_expect {Available commands:}
must_expect {\\(psh\\)%}
send "\\003"
expect eof
' > $log_path 2>&1
rc=\$?
grep -En "Available commands:|\\(psh\\)%|help" $log_path | tail -n 20 || true
printf "Log: %s\\n" $log_path
exit \$rc
EOF
)

limactl shell -y "$vm" -- /bin/bash -lc "$remote_cmd"
