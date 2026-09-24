#!/bin/bash
# For each page+4 fire in a UART log, decide whether that page was EVER a V3D BO.
#
# This is the discriminator between the two surviving readings of C1:
#
#   (a) the write targets a closed BO's pages -- the fired page falls inside some
#       `v3d-bo: CREATE/CLOSE ... cpu=<p> size=<n>` range, and the closed-BO route
#       is real;
#   (b) the write targets ordinary malloc memory, and V3D_KEEP_CLOSED_BO /the
#       quarantine only ever changed the RATE because they stop closed-BO pages
#       going back to the kernel, which changes which pages malloc is handed. Then
#       the fired page was never a BO, and this is a CPU-side use-after-free.
#
# Needs a build with V3D_BO_TRACE default-on (the C1 hunt build). The winsys is
# in-process with malloc, so the addresses are directly comparable.
#
# ⚠ A "never a BO" answer is only as good as the trace: if the log shows zero
# CREATE lines the trace was off and the answer is VOID, not (b). That check is
# first, deliberately.
#
# Usage: scripts/c1-was-it-a-bo.sh [label|path]   (default: newest log)
set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ART="$REPO/artifacts/rpi4b-uart"

arg="${1:-}"
if [ -n "$arg" ] && [ -f "$arg" ]; then
	log="$arg"
elif [ -n "$arg" ]; then
	log="$(ls -t "$ART"/*"$arg"*.log 2>/dev/null | head -1)"
else
	log="$(ls -t "$ART"/*.log 2>/dev/null | head -1)"
fi
[ -n "${log:-}" ] && [ -f "$log" ] || { echo "no log found for '${arg}'" >&2; exit 1; }

echo "log: $(basename "$log")"

traced=$(grep -ac 'v3d-bo: CREATE' "$log")
fires=$(grep -ac 'p4page = ' "$log")
echo "  BO trace lines: $traced ; page+4 fires: $fires"
if [ "$traced" -eq 0 ]; then
	echo "  => VOID: the BO trace was OFF in this build, so 'never a BO' cannot be concluded."
	exit 0
fi
if [ "$fires" -eq 0 ]; then
	echo "  => nothing fired this run; nothing to attribute."
	exit 0
fi
echo

python3 - "$log" <<'PY'
import re, sys

log = sys.argv[1]
raw = open(log, 'rb').read().decode('utf-8', 'replace')

# BO ranges, from both CREATE and CLOSE (a BO's cpu mapping is the same in each).
bo = []
for m in re.finditer(r'v3d-bo: (CREATE|CLOSE)\s+handle=(\d+).*?size=(\d+)\s+cpu=(0x[0-9a-fA-F]+)', raw):
    kind, handle, size, cpu = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4), 16)
    bo.append((cpu, cpu + size, handle, kind))

pages = [int(m.group(1), 16) for m in re.finditer(r'p4page = (0x[0-9a-fA-F]+)', raw)]

print(f"  {len(bo)} traced BO ranges, {len(pages)} fired page(s)")
print()
verdicts = []
for p in pages:
    hits = [(h, k, lo) for (lo, hi, h, k) in bo if lo <= p < hi]
    if hits:
        h, k, lo = hits[0]
        print(f"  page 0x{p:x}  ->  INSIDE BO handle={h} ({k}, base 0x{lo:x}, +0x{p-lo:x})")
        verdicts.append('bo')
    else:
        # how close did we get? a near miss is worth seeing.
        best = min(((min(abs(p-lo), abs(p-hi)), h) for (lo, hi, h, k) in bo), default=(None, None))
        near = f", nearest traced BO {best[1]} is 0x{best[0]:x} away" if best[0] is not None else ""
        print(f"  page 0x{p:x}  ->  NEVER A BO{near}")
        verdicts.append('notbo')

print()
if all(v == 'notbo' for v in verdicts):
    print("  => READING (b): every fired page was ordinary malloc memory, never a BO.")
    print("     The closed-BO route is excluded; this looks like a CPU-side use-after-free.")
elif all(v == 'bo' for v in verdicts):
    print("  => READING (a): every fired page was a BO. The closed-BO route is REAL.")
else:
    print("  => MIXED -- some fired pages were BOs and some were not. Report both.")
PY
