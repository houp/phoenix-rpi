#!/usr/bin/env bash
#
# radio-ap-down.sh — tear down the phoenix-ap WiFi access point created by
# radio-ap-up.sh, restoring wlp3s0 to normal NetworkManager management.
set -euo pipefail
CON="phoenix-ap"
nmcli connection down "$CON" >/dev/null 2>&1 || true
nmcli connection delete "$CON" >/dev/null 2>&1 || true
echo "radio-ap: DOWN — '$CON' stopped + removed; wlp3s0 back to NM management"
