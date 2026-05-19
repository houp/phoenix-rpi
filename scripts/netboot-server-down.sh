#!/usr/bin/env bash
#
# Stop the Pi 4 netboot dnsmasq. On macOS this shells into the Lima VM;
# on Linux it stops the dnsmasq running directly on the host. Either
# way the actual stop logic lives in scripts/vm-netboot-server.sh.
#

set -euo pipefail

repo="${PHOENIX_RPI_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
host_os="$(uname -s)"

case "$host_os" in
	Darwin)
		vm="${PHOENIX_VM:-phoenix-dev}"
		if ! limactl list -f '{{.Name}} {{.Status}}' | grep -q "^${vm} Running$"; then
			printf 'VM "%s" not running; nothing to stop.\n' "$vm"
			exit 0
		fi
		limactl shell -y "$vm" -- /Users/witoldbolt/phoenix-rpi/scripts/vm-netboot-server.sh down
		;;
	Linux)
		export PHOENIX_BUILDROOT="${PHOENIX_BUILDROOT:-$repo/.buildroot}"
		export RPI4B_NETBOOT_STATE_DIR="${RPI4B_NETBOOT_STATE_DIR:-$repo/artifacts/netboot}"
		exec "$repo/scripts/vm-netboot-server.sh" down
		;;
	*)
		printf 'unsupported host OS: %s\n' "$host_os" >&2
		exit 1
		;;
esac
