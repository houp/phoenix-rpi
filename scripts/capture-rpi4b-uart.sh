#!/usr/bin/env bash

set -euo pipefail

default_image_path="/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img"
default_output_dir="/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart"

baud="115200"
device=""
exit_after=""
label=""
list_only=0
log_path=""
output_dir="$default_output_dir"

usage() {
	cat <<'EOF'
Usage: capture-rpi4b-uart.sh [options]

Options:
  --device PATH
      explicit serial device, for example /dev/cu.usbserial-0001
  --baud N
      baud rate, default 115200
  --log PATH
      explicit log path
  --output-dir PATH
      directory for auto-created logs
  --label TEXT
      short label added to the auto-created log filename
  --list
      list candidate macOS USB serial devices and exit
  --exit-after MSEC
      pass picocom --exit-after for dry runs or tests
  --help
      show this help

Notes:
  - current Raspberry Pi UART lane expects 115200 8N1
  - preferred device path form on macOS is /dev/cu.*
  - exit picocom with Ctrl-A Ctrl-X
EOF
}

list_candidates() {
	local dev base seen=""

	for dev in /dev/cu.*; do
		[ -e "$dev" ] || continue
		base="$(basename "$dev")"
		case "$base" in
			*usb*|*USB*|*serial*|*Serial*|*SLAB*|*wch*|*UART*|*uart*|*modem*|*FT*)
				case " $seen " in
					*" $dev "*) ;;
					*)
						printf '%s\n' "$dev"
						seen="$seen $dev"
						;;
				esac
				;;
		esac
	done
}

autodetect_device() {
	local count=0 candidate=""

	while IFS= read -r candidate; do
		[ -n "$candidate" ] || continue
		device="$candidate"
		count=$((count + 1))
	done <<EOF
$(list_candidates)
EOF

	if [ "$count" -eq 0 ]; then
		printf 'no candidate USB serial devices found\n' >&2
		printf 'hint: try --list after plugging the adapter in\n' >&2
		exit 1
	fi

	if [ "$count" -gt 1 ]; then
		printf 'multiple candidate USB serial devices found\n' >&2
		list_candidates >&2
		printf 'hint: rerun with --device /dev/cu....\n' >&2
		exit 1
	fi
}

safe_label() {
	printf '%s' "$1" | tr ' /' '__' | tr -cd '[:alnum:]_.-'
}

while [ $# -gt 0 ]; do
	case "$1" in
		--device)
			device="$2"
			shift 2
			;;
		--baud)
			baud="$2"
			shift 2
			;;
		--log)
			log_path="$2"
			shift 2
			;;
		--output-dir)
			output_dir="$2"
			shift 2
			;;
		--label)
			label="$2"
			shift 2
			;;
		--list)
			list_only=1
			shift
			;;
		--exit-after)
			exit_after="$2"
			shift 2
			;;
		--help|-h)
			usage
			exit 0
			;;
		*)
			printf 'unknown option: %s\n' "$1" >&2
			usage >&2
			exit 1
			;;
	esac
done

if [ "$list_only" -eq 1 ]; then
	list_candidates
	exit 0
fi

if [ -z "$device" ]; then
	autodetect_device
fi

if [ ! -e "$device" ]; then
	printf 'missing serial device: %s\n' "$device" >&2
	exit 1
fi

mkdir -p "$output_dir"

if [ -z "$log_path" ]; then
	timestamp="$(date +%Y%m%d-%H%M%S)"
	log_name="rpi4b-uart-$timestamp"
	if [ -n "$label" ]; then
		log_name="${log_name}-$(safe_label "$label")"
	fi
	log_path="${output_dir}/${log_name}.log"
fi

meta_path="${log_path}.meta.txt"
image_sha256="unknown"

if [ -f "$default_image_path" ]; then
	image_sha256="$(shasum -a 256 "$default_image_path" | awk '{print $1}')"
fi

cat > "$meta_path" <<EOF
timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)
serial_device: $device
baud: $baud
image_path: $default_image_path
image_sha256: $image_sha256
note: use Ctrl-A Ctrl-X to exit picocom cleanly
EOF

printf 'UART device: %s\n' "$device"
printf 'UART baud:   %s\n' "$baud"
printf 'UART log:    %s\n' "$log_path"
printf 'UART meta:   %s\n' "$meta_path"
printf 'Exit:        Ctrl-A Ctrl-X\n'

cmd=(
	picocom
	--baud "$baud"
	--flow n
	--parity n
	--databits 8
	--stopbits 1
	--noinit
	--noreset
	--logfile "$log_path"
)

if [ -n "$exit_after" ]; then
	cmd+=(--exit-after "$exit_after")
fi

cmd+=("$device")

exec "${cmd[@]}"
