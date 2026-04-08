#!/usr/bin/env bash

set -euo pipefail

image_path="${RPI4B_SDIMG_PATH:-/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img}"
expected_sha256="${RPI4B_SDIMG_SHA256:-1c3bc4f6c474baad547059801ba49ea4c2de31c088aea3b1ef68fc7b8eb2924f}"
expected_size="${RPI4B_SDIMG_SIZE:-69206016}"

if [ ! -f "$image_path" ]; then
	printf 'missing image: %s\n' "$image_path" >&2
	exit 1
fi

actual_size="$(wc -c < "$image_path" | tr -d '[:space:]')"
actual_sha256="$(shasum -a 256 "$image_path" | awk '{print $1}')"

printf 'Image: %s\n' "$image_path"
printf 'Size:  %s\n' "$actual_size"
printf 'SHA256: %s\n' "$actual_sha256"

if [ "$actual_size" != "$expected_size" ]; then
	printf 'size mismatch: expected %s\n' "$expected_size" >&2
	exit 1
fi

if [ "$actual_sha256" != "$expected_sha256" ]; then
	printf 'sha256 mismatch: expected %s\n' "$expected_sha256" >&2
	exit 1
fi

printf 'Verification: OK\n'
