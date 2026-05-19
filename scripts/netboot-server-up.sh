#!/usr/bin/env bash
#
# Bring up the Pi 4 netboot server. Cross-host: on macOS this shells into
# the phoenix-dev Lima VM; on Linux it runs the dnsmasq directly on the
# host. The actual dnsmasq is launched by scripts/vm-netboot-server.sh —
# this wrapper just dispatches.
#
# macOS topology:
#   Pi 4 ETH ─── en7 (USB-C) ═══ socket_vmnet ═══ lima1 (in VM) → dnsmasq
#
# Linux topology:
#   Pi 4 ETH ─── dedicated NIC ────────────────────────────────→ dnsmasq
#                  (eth1 / enp0s2 / similar)
#
# Sibling scripts:
#   scripts/netboot-server-down.sh    stop the dnsmasq
#   scripts/test-cycle-netboot.sh     one full power-cycle + UART capture
#   scripts/vm-netboot-server.sh      the worker that actually runs dnsmasq
#

set -euo pipefail

repo="${PHOENIX_RPI_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
host_os="$(uname -s)"

case "$host_os" in
	Darwin)
		# macOS path: shell into the Lima VM
		vm="${PHOENIX_VM:-phoenix-dev}"
		if ! command -v limactl >/dev/null 2>&1; then
			printf 'limactl not found on PATH\n' >&2
			exit 1
		fi
		if ! limactl list -f '{{.Name}} {{.Status}}' | grep -q "^${vm} Running$"; then
			printf 'VM "%s" is not running. Start it with: limactl start %s\n' "$vm" "$vm" >&2
			exit 1
		fi
		# Pass through any RPI4B_NETBOOT_* overrides
		env_args=()
		while IFS='=' read -r k v; do
			case "$k" in
				RPI4B_NETBOOT_*|PHOENIX_BUILDROOT) env_args+=("$k=$v") ;;
			esac
		done < <(env)
		limactl shell -y "$vm" -- env ${env_args[@]+"${env_args[@]}"} \
			/Users/witoldbolt/phoenix-rpi/scripts/vm-netboot-server.sh up
		;;
	Linux)
		# Linux path: invoke the worker directly. The worker reads
		# RPI4B_NETBOOT_* env vars to override interface, IP range, etc.
		# Sensible Linux defaults below; override via env or .env.local.
		export RPI4B_NETBOOT_IFACE="${RPI4B_NETBOOT_IFACE:-eth1}"
		export PHOENIX_BUILDROOT="${PHOENIX_BUILDROOT:-$repo/.buildroot}"
		export RPI4B_NETBOOT_TFTPROOT="${RPI4B_NETBOOT_TFTPROOT:-$PHOENIX_BUILDROOT/_boot/aarch64a72-generic-rpi4b/rpi4b-bootfs}"
		export RPI4B_NETBOOT_STATE_DIR="${RPI4B_NETBOOT_STATE_DIR:-$repo/artifacts/netboot}"
		exec "$repo/scripts/vm-netboot-server.sh" up
		;;
	*)
		printf 'unsupported host OS: %s\n' "$host_os" >&2
		exit 1
		;;
esac
