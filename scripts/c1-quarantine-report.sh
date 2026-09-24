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
# NB on verifying a build actually shipped: the binary that carries the winsys is
# /usr/bin/supertuxkart, NOT /bin/stk (a launcher). Checking the wrong one reports
# a stale build as missing the change. And use `rg --no-ignore` -- our grep honours
# .gitignore, so a -r search under .buildroot silently finds nothing.
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
mmu=$(grep -acE 'PT_INVALID|MMU_VIO|PTI_ABORT|mmu.*abort' "$log")

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

# A quarantine run is only interpretable as a PAIR of numbers: the heap tripwire
# says whether a C1 event happened at all this run, and the quarantine says
# whether it came through a closed BO's pages. Either alone is unreadable --
# strays=0 on a run that never fired proves nothing, which is the whole reason
# this line exists.
echo "== VERDICT (read p4 and strays TOGETHER) =="
if [ -z "$flips" ]; then
	v="VOID -- workload did not run"
elif [ "$found" -eq 0 ]; then
	v="VOID -- the scan never found its own plant"
elif [ "$p4" -gt 0 ] && [ "$strays" -gt 0 ]; then
	v="ROUTE CONFIRMED -- fired, and a closed BO was written"
elif [ "$p4" -gt 0 ] && [ "$strays" -eq 0 ]; then
	v="ROUTE EXCLUDED -- fired, but NO closed BO was written (points away from the BO pages)"
elif [ "$p4" -eq 0 ] && [ "$strays" -gt 0 ]; then
	v="a closed BO was written without a heap fire -- interesting, chase it"
else
	v="QUIET -- no event this run; proves nothing either way"
fi
echo "  p4=$p4  strays=$strays  flagged=${flagged:-none}  frames=${flips:-none}"
echo "  => $v"
echo

echo "== side-effects of the instrument =="
echo "  MMU/PT aborts   : $mmu   (>0 with quarantine on = Mesa is reading BOs it closed)"
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
