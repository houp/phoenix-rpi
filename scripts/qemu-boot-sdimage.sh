#!/usr/bin/env bash
#
# Boot a flashable SD image's loader under QEMU — a structural boot check for the
# artifact the owner actually flashes.
#
# Why: the SD boot path cannot be exercised on this host (the card reader shares
# the USB-C port with power/eth and is detached), so every image so far has been
# gated only on CONTENTS (verify-sd-image-contents.sh) and on netboot runs of
# byte-identical binaries. Nothing has ever taken the image itself through a boot.
# This closes part of that gap without a card: it pulls loader.disk out of the
# image's FAT partition with mtools (no root needed) and boots it under QEMU using
# the live build's plo.elf, since plo does not differ between the sd/nfsroot/
# netboot variants -- loader.disk is what carries the syspage and program list, so
# swapping only that isolates the image's own contribution.
#
# WHAT THIS PROVES: the image's FAT partition holds a plo-loadable loader.disk,
# plo parses its pre-init script, and control reaches kernel entry. It also
# reports whether the syspage is the SD variant.
#
# WHAT IT DOES NOT PROVE: that the kernel runs (QEMU's rpi4 model stops it almost
# immediately), that the ext2 root mounts, that EMMC2/SD works, or that userspace
# starts. This is NOT an SD-boot gate -- it is a check that the image is not
# structurally broken.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
img="${1:-$repo/artifacts/rpi4b/rpi4b-sd-2part.img}"
fat_offset="${RPI4B_FAT_OFFSET:-1048576}"

command -v mcopy >/dev/null 2>&1 || { echo "qemu-boot-sdimage: mtools (mcopy) not found" >&2; exit 1; }
[ -f "$img" ] || { echo "qemu-boot-sdimage: no such image: $img" >&2; exit 1; }

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

printf 'qemu-boot-sdimage: image  %s\n' "$img"
printf 'qemu-boot-sdimage: sha256 %s\n' "$(sha256sum "$img" | cut -d' ' -f1)"

mcopy -i "${img}@@${fat_offset}" ::loader.disk "$tmp/loader.disk" \
	|| { echo "qemu-boot-sdimage: could not extract loader.disk from the FAT partition" >&2; exit 1; }

nfs_n="$(strings "$tmp/loader.disk" | grep -c 'nfs;/' || true)"
emmc_n="$(strings "$tmp/loader.disk" | grep -c 'bcm2711-emmc' || true)"
printf 'qemu-boot-sdimage: syspage: nfs;/ refs=%s  bcm2711-emmc refs=%s' "$nfs_n" "$emmc_n"
# ⚠ This was advisory-only until 2026-09-17: it printed the warning and then
# still exited 0 on "plo reached kernel entry", so an SD image carrying a
# NETBOOT loader passed the one automated check that inspects it — and that
# image hangs at first boot with no network (see verify-sd-image-contents.sh).
# A wrong variant is a failure, not a note.
variant_bad=0
if [ "$nfs_n" = "0" ] && [ "$emmc_n" != "0" ]; then
	printf '  -> SD variant, as expected\n'
else
	printf '  -> ⚠️ NOT the SD variant (an sd image must have no nfs;/ and must have bcm2711-emmc)\n'
	variant_bad=1
fi

log_label="sdimage-$(basename "$img" .img)"
# Time bound for the log pickup below: "newest log matching this label" with no
# lower bound silently grades a PREVIOUS run of the same image when this run
# produces nothing.
touch "$tmp/started"
QEMU_LOADER="$tmp/loader.disk" "$repo/scripts/qemu-debug.sh" \
	--timeout "${RPI4B_QEMU_TIMEOUT:-45}" --label "$log_label" >/dev/null 2>&1 || true

log="$(find "$repo/artifacts/qemu" -maxdepth 1 -name "*${log_label}*.uart.log" \
	-newer "$tmp/started" -printf '%T@ %p\n' 2>/dev/null | sort -rn | head -1 | cut -d' ' -f2-)"
if [ -z "$log" ]; then
	echo "qemu-boot-sdimage: FAIL — no QEMU UART log produced by THIS run" >&2
	exit 1
fi

printf 'qemu-boot-sdimage: uart log %s\n' "$log"
if grep -q 'kernel entry' "$log"; then
	if [ "$variant_bad" = 1 ]; then
		echo "qemu-boot-sdimage: FAIL — plo reached kernel entry, but this is NOT an SD" >&2
		echo "  variant (nfs;/ refs=$nfs_n, bcm2711-emmc refs=$emmc_n). Flashed to a card" >&2
		echo "  it would hang at first boot looking for a network root." >&2
		exit 1
	fi
	printf 'qemu-boot-sdimage: PASS — plo parsed the image loader and reached kernel entry\n'
	exit 0
fi
echo "qemu-boot-sdimage: FAIL — plo did not reach kernel entry; log tail:" >&2
tail -8 "$log" >&2
exit 1
