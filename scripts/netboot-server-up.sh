#!/usr/bin/env bash
#
# Bring up the Pi 4 netboot server on this Linux host: a dnsmasq instance
# serving DHCP + TFTP on the dedicated USB-Ethernet NIC, straight to the Pi 4
# over a crossover cable. No VM, no bridge.
#
#   Pi 4 ETH ─── USB NIC (RPI4B_NETBOOT_IFACE) ──────────────→ dnsmasq
#
# The actual dnsmasq is launched by scripts/netboot-server.sh (the worker);
# this wrapper sets sensible Linux defaults and dispatches. Re-running is safe —
# the worker stops any existing instance and starts fresh.
#
# Config comes from .env.local (gitignored), e.g.:
#   RPI4B_NETBOOT_IFACE=enx00e04c68013a
#
# Sibling scripts:
#   scripts/netboot-server-down.sh     stop the dnsmasq
#   scripts/netboot-server-restart.sh  re-up the NIC + restart dnsmasq fresh
#   scripts/test-cycle-netboot.sh      one full power-cycle + UART capture
#   scripts/netboot-server.sh          the worker that actually runs dnsmasq
#

set -euo pipefail

repo="${PHOENIX_RPI_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"

# Honour project-local environment overrides (e.g. RPI4B_NETBOOT_IFACE
# pointing at the actual USB-Ethernet device name). .env.local is gitignored.
if [ -f "$repo/.env.local" ]; then
	set -a
	# shellcheck disable=SC1091
	. "$repo/.env.local"
	set +a
fi

export RPI4B_NETBOOT_IFACE="${RPI4B_NETBOOT_IFACE:-eth1}"
export PHOENIX_BUILDROOT="${PHOENIX_BUILDROOT:-$repo/.buildroot}"
export RPI4B_NETBOOT_TFTPROOT="${RPI4B_NETBOOT_TFTPROOT:-$PHOENIX_BUILDROOT/_boot/aarch64a72-generic-rpi4b/rpi4b-bootfs}"
export RPI4B_NETBOOT_STATE_DIR="${RPI4B_NETBOOT_STATE_DIR:-$repo/artifacts/netboot}"

# Keep the NFS root in step with the freshly-built kernel: sync the base rootfs into the export
# (preserving hand-staged games/assets) so userspace matches the TFTP-served kernel. Without this
# the export drifts and stale userspace hits syscall/errno ABI mismatches vs the fresh kernel.
"$repo/scripts/sync-netboot-tree.sh" || printf 'netboot-server-up.sh: WARN sync-netboot-tree failed (continuing)\n'

# E2: enable outbound internet for the netboot Pi (host NAT for the lab subnet).
# Idempotent + additive + non-fatal (netboot works fine without it). Pairs with the
# DHCP router/DNS options (option 3/6) baked into netboot-server.sh so the Pi
# auto-configures gateway+DNS and can reach the internet. See project_pi4_internet_e2.
"$repo/scripts/pi-internet-nat.sh" || printf 'netboot-server-up.sh: WARN pi-internet-nat failed (Pi internet unavailable; netboot OK)\n'

exec "$repo/scripts/netboot-server.sh" up
