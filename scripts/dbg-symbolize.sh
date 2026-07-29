#!/usr/bin/env bash
# dbg-symbolize.sh — turn dbg backtrace return addresses into function:line.
#
# Usage:
#   scripts/dbg-symbolize.sh <elf> <addr> [addr ...]
#   scripts/dbg-symbolize.sh <elf> < <log-with-dbg-lines>   # reads 0x... from stdin
#
# Pairs with tools/dbg-probe/dbg.c: the on-target facility prints
#   dbg:   #LEAF 0x....
#   dbg:   #NN 0x....
# over UART; feed those addresses here (host-side) against the exact ELF that ran.
set -euo pipefail

A2L=".toolchain/aarch64-phoenix/bin/aarch64-phoenix-addr2line"
[ -x "$A2L" ] || A2L="$(command -v aarch64-phoenix-addr2line || true)"
[ -n "$A2L" ] || { echo "addr2line for aarch64-phoenix not found" >&2; exit 1; }

elf="${1:?usage: dbg-symbolize.sh <elf> [addr ...]}"
shift || true

addrs=("$@")
if [ "${#addrs[@]}" -eq 0 ]; then
	# Scan stdin for 0x... tokens (e.g. a pasted/grep'd dbg backtrace).
	mapfile -t addrs < <(grep -aoE '0x[0-9a-fA-F]+' || true)
fi
[ "${#addrs[@]}" -gt 0 ] || { echo "no addresses given" >&2; exit 1; }

for a in "${addrs[@]}"; do
	printf '%s -> ' "$a"
	"$A2L" -f -e "$elf" "$a" | tr '\n' ' '
	printf '\n'
done
