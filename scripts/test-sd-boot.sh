#!/usr/bin/env bash
#
# test-sd-boot.sh — boot Phoenix from the Pi's SD CARD (not netboot) and grade it.
#
# This is the one lane this bench has never exercised. Everything else — every
# gate, every soak, every reel capture — runs over netboot with an NFS root. The
# SD image is the artifact the owner actually presents from, and until this runs
# nobody has watched it boot.
#
# HOW SD BOOT IS SELECTED. The firmware falls back to the card when the netboot
# server is DOWN, so this script brings dnsmasq down, runs the cycle with
# `--sd-boot` (which skips server-up AND zeroes the DHCP watchdog — without it the
# watchdog's bridge recovery restarts dnsmasq mid-cycle and the Pi netboots after
# all), then brings the server back up on EVERY exit path. That restore is the
# critical bit: netboot is the only other lane, so leaving it down would strand
# the bench.
#
# ⚠ PRECONDITION: a card holding a bootable Phoenix image must be in the Pi.
# Measured 2026-09-16: with a BLANK card inserted the Pi does not boot at all —
# no DHCP, no UART, black HDMI — so this script cannot be used to bring up a card
# that has not already been written. See docs/inprogress WEEK log.
#
# WHAT IT CHECKS, beyond "it booted":
#   1. psh prompt + 0 faults, from uart-summary.sh.
#   2. The SD shader cache. The V3D driver writes its blob cache to a CWD-RELATIVE
#      `./.mesa-shader-cache/v1`, which on the SD lane should land on the
#      persistent ext2 root at `/.mesa-shader-cache/v1`. That has never been
#      observed (docs/misc/2026-09-16-shader-cache-map.md). Boot 1 runs a GL app to
#      populate it and lists it; boot 2 lists it again WITHOUT running the app. If
#      the second listing still shows blobs, the cache genuinely persists across an
#      SD reboot and a presenter pays the ~67 s vkQuake cold start only once per
#      card. If it is empty or absent, every GPU app start on the demo image is
#      cold, which is a presentation-grade finding.
#   ⓘ A first GL run on a fresh root MUST print `v3d: shader cache COLD (...)`.
#      Absence of that line does not mean "warm" — disk_cache_create returns NULL
#      (caching silently OFF) before printing it if the mkdir fails, e.g. on a
#      read-only or full root. Grade the blob listing, not the silence.
#
# Usage: ./scripts/test-sd-boot.sh [label]
#
# Copyright 2026 Phoenix Systems
# SPDX-License-Identifier: BSD-3-Clause
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:-sdboot}"
cd "$repo"

restore_netboot() {
	echo ""
	echo "=== restoring the netboot server (this bench has no other lane) ==="
	"$repo/scripts/netboot-server-up.sh" >/dev/null 2>&1 \
		&& echo "netboot server: UP" \
		|| echo "!! netboot server did NOT come back up — run scripts/netboot-server-up.sh by hand" >&2
}
trap restore_netboot EXIT INT TERM

echo "=== bringing the netboot server DOWN so the firmware falls back to the card ==="
"$repo/scripts/netboot-server-down.sh" 2>&1 | tail -3

run_cycle() {
	local lbl="$1"; shift
	if [ "$#" -gt 0 ]; then
		"$repo/scripts/test-cycle-psh-interact.sh" --skip-server-up \
			--label "$lbl" --wait-secs 200 --idle-secs 90 --max-cmd-secs 120 -- "$@"
	else
		"$repo/scripts/test-cycle-netboot.sh" --sd-boot --label "$lbl" --capture-secs 200
	fi
}

# ---- boot 1: does it come up at all, and does a GL app populate the cache? ----
echo ""
echo "=== boot 1/2: SD boot + populate the shader cache ==="
run_cycle "${label}-b1" \
	"uname -a" \
	"quakespasm" \
	"ls -la /" \
	"ls /.mesa-shader-cache/v1"

# ---- boot 2: is the cache still there WITHOUT running the app? ----
echo ""
echo "=== boot 2/2: SD reboot, list the cache without repopulating it ==="
run_cycle "${label}-b2" \
	"ls -la /" \
	"ls /.mesa-shader-cache/v1"

echo ""
echo "=== grading ==="
for b in b1 b2; do
	log="$(ls -t "${repo}"/artifacts/rpi4b-uart/*"${label}-${b}"*.log 2>/dev/null | head -1)"
	[ -n "${log}" ] || { echo "${b}: NO LOG"; continue; }
	echo "--- ${b}: $(basename "${log}")"
	"$repo/scripts/uart-summary.sh" "${log}" 2>/dev/null \
		| grep -aE "psh prompt|fault_pattern_matches|ends_mid_line" || true
	# grep -a: the logs carry binary bytes.
	cold=$(grep -ac "shader cache COLD" "${log}")
	blobs=$(grep -aoE "^[0-9a-f]{64}$" "${log}" | wc -l | tr -d ' ')
	echo "    shader cache: COLD reports=${cold}  blob filenames listed=${blobs}"
done

echo ""
echo "VERDICT is a HUMAN read of the two blob counts above:"
echo "  b1 > 0 and b2 > 0  => the cache persists across an SD reboot (the good case)"
echo "  b1 > 0 and b2 == 0 => it is written but lost on reboot"
echo "  b1 == 0            => it never lands on the SD root at all; every GPU app"
echo "                        start on the demo image pays the full cold compile"
echo "Also LOOK at artifacts/hdmi/*${label}* — a clean log does not mean it drew."
