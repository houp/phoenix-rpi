#!/bin/bash
# Grade a C1 closed-BO quarantine run (V3D_BO_QUARANTINE, V3D_CL_CACHE_CLEAN).
#
# The whole value of this instrument is that its SILENCE is meaningful, so the
# first thing printed is not the result but the four assertions that have to hold
# before a silent run may be read as "nothing wrote a closed BO":
#
#   1. the workload ran            -- memtrip flip count advanced
#   2. the instrument was ON       -- the driver printed its own banner
#   3. the scan actually works     -- it found the word planted in this same run
#   4. the hypothesis is testable  -- Mesa produced FLUSH_CACHE-flagged jobs
#
# Assertion 3 is the selftest: every run plants the exact C1 word in the first
# quarantined BO and the scan has to find it. A run whose scan never found its own
# plant proves nothing whatever it reports.
#
# Assertion 4 is the one that can kill the idea outright: the standing hypothesis
# is a TMU write combiner draining late, and only a job whose shaders wrote
# through the TMU can park anything there. Zero flagged jobs means this workload
# has no such writer and the quarantine's silence says nothing at all.
#
# Usage: scripts/c1-quarantine-report.sh [label|path]   (default: newest log)
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
echo

banner=$(grep -ac 'closed-BO quarantine ON' "$log")
exitline=$(grep -a 'v3d-qt: EXIT' "$log" | tail -1)
released=$(grep -ac 'v3d-qt: released' "$log")
strays=$(grep -ac 'v3d-qt: STRAY WRITE' "$log")
planted=$(grep -ac 'v3d-qt: selftest -- planted' "$log")
found=$(grep -ac 'v3d-qt: SELFTEST PLANT found' "$log")
flagged=$(grep -a 'CL FLUSH_CACHE job #' "$log" | tail -1)
cleaning=$(grep -ac 'CL FLUSH_CACHE job #.*-- cleaning' "$log")
flips=$(grep -ao 'total [0-9]*)' "$log" | tail -1)
p4=$(grep -ac 'PAGE+4 POISON BROKEN' "$log")

say() { # say <ok?> <text>
	if [ "$1" = 1 ]; then echo "  [OK]   $2"; else echo "  [FAIL] $2"; fi
}

echo "== assertions (all must hold before silence means anything) =="
say "$([ -n "$flips" ] && echo 1 || echo 0)" "workload ran            : memtrip ${flips:-<NO FLIP COUNT -- VOID RUN>}"
say "$([ "$banner" -gt 0 ] && echo 1 || echo 0)" "quarantine ON           : driver banner x$banner"
say "$([ -n "$exitline" ] && echo 1 || echo 0)" "quarantine did work     : ${exitline:-<no EXIT summary -- app did not exit cleanly>}"
say "$([ "$found" -gt 0 ] && echo 1 || echo 0)" "scan works this run     : planted x$planted, found x$found"
say "$([ -n "$flagged" ] && echo 1 || echo 0)" "hypothesis testable     : ${flagged:-<NO FLUSH_CACHE-FLAGGED JOBS -- no TMU writer, silence proves nothing>}"
echo

echo "== arm =="
if [ "$cleaning" -gt 0 ]; then
	echo "  ARM B: V3D_CL_CACHE_CLEAN=1  (TMUWCF drain + L2T clean, both awaited)"
elif [ -n "$flagged" ]; then
	echo "  ARM A: legacy behaviour      (flag advertised, never honoured)"
else
	echo "  <indeterminate -- no flagged job seen>"
fi
echo

echo "== result =="
echo "  stray writes into closed BOs : $strays"
echo "  release heartbeats           : $released"
echo "  selftest plant found         : $found of $planted"
echo "  heap page+4 fires            : $p4   (NOT comparable across quarantine/non-quarantine builds)"
echo

if [ "$strays" -gt 0 ]; then
	echo "== the stray writes =="
	grep -a -A9 'v3d-qt: STRAY WRITE' "$log" | head -60
fi
