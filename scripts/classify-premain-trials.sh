#!/usr/bin/env bash
# Classify the trials of a premain-hang bench by HOW FAR the launched process got.
#
# The `premain-hang` signature is a launch that prints NOTHING -- not even the
# app's own first line. The open question is whether the process ever reached
# libc at all. `LIBC_STARTUP_TRACE=min` (libphoenix misc/init.c) answers it by
# printing exactly one marker, `libc-init: enter`, as the first statement of
# `_libc_init()`:
#
#   command echo, then NOTHING          -> never reached _libc_init: the fault is
#                                          in exec / program loading (KERNEL side)
#   marker, then no `main() entered`    -> died inside a libc initialiser
#   marker + main + app output          -> healthy
#
# A run is only EVIDENCE if the trial actually reached a psh prompt and the
# command was echoed; a truncated capture is neither a pass nor a failure, and
# counting it as either is the mistake the W36 counting rule exists to prevent.
#
# One more class is essential and easy to miss: a trial where psh reached its
# prompt but the command was NEVER ECHOED. There the command did not reach the
# shell at all, so the app was never launched -- it is harness noise, not a
# premain event. Counting it as one inflates the rate (it is what made a 2/23
# bench read as 3/24). Pass <cmd-echo-ERE> to separate it.
#
# Usage: ./scripts/classify-premain-trials.sh <label> [app-first-line-ERE] [cmd-echo-ERE]
#        default app marker: 'main\(\) entered'

set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:?usage: classify-premain-trials.sh <label> [app-first-line-ERE]}"
app_re="${2:-main\(\) entered}"
cmd_re="${3:-}"

shopt -s nullglob
logs=( "${repo_root}/artifacts/rpi4b-uart/"rpi4b-uart-*-"${label}"-T*.log )
if [ ${#logs[@]} -eq 0 ]; then
	printf 'classify-premain-trials: no logs for label "%s"\n' "$label" >&2
	exit 1
fi

n_silent=0 n_libc_only=0 n_ok=0 n_void=0 n_nocmd=0

# Does this build carry the instrument AT ALL? On an untraced build a failing
# launch also prints zero markers, so reading that as "never reached _libc_init"
# would turn the absence of the instrument into evidence about the fault. Decide
# per-BENCH: if no trial anywhere printed a marker, the build is untraced and no
# pre-main verdict can be drawn from marker counts.
traced=0
for log in "${logs[@]}"; do
	if grep -aq 'libc-init: enter' "$log"; then traced=1; break; fi
done

printf '%-52s %-12s %s\n' "LOG" "CLASS" "DETAIL"
printf '%-52s %-12s %s\n' "----------------------------------------------------" "------------" "------"
for log in "${logs[@]}"; do
	prompt=$(grep -ac '(psh)%' "$log" || true)
	marker=$(grep -ac 'libc-init: enter' "$log" || true)
	app=$(grep -acE "$app_re" "$log" || true)
	if [ -n "$cmd_re" ]; then
		echoed=$(grep -acE "$cmd_re" "$log" || true)
	else
		echoed=1
	fi
	if [ "$prompt" -eq 0 ]; then
		cls="VOID"; detail="no psh prompt -- not evidence"; n_void=$(( n_void + 1 ))
	elif [ "$echoed" -eq 0 ]; then
		cls="NO-CMD"; detail="command never echoed -- never launched, not evidence"
		n_nocmd=$(( n_nocmd + 1 ))
	elif [ "$app" -gt 0 ]; then
		cls="OK"; detail="marker=${marker} app=${app}"; n_ok=$(( n_ok + 1 ))
	elif [ "$marker" -gt 0 ]; then
		cls="LIBC-INIT"; detail="reached _libc_init, never reached main()"
		n_libc_only=$(( n_libc_only + 1 ))
	else
		cls="SILENT"; detail="0 markers -- never reached _libc_init (exec/loading)"
		n_silent=$(( n_silent + 1 ))
	fi
	printf '%-52s %-12s %s\n' "$(basename "$log")" "$cls" "$detail"
done

printf '\n=== %s: %d trials -- OK %d | SILENT %d | LIBC-INIT %d | NO-CMD %d | VOID %d ===\n' \
	"$label" "${#logs[@]}" "$n_ok" "$n_silent" "$n_libc_only" "$n_nocmd" "$n_void"
if [ -z "$cmd_re" ]; then
	printf 'NOTE: no <cmd-echo-ERE> given, so a trial where the command never reached\n'
	printf '      the shell is counted as SILENT. Pass one to separate that class.\n'
fi
if [ "$traced" -eq 0 ]; then
	printf 'VERDICT: none possible -- NO trial printed a marker, so this bench ran an\n'
	printf '         UNTRACED build. Rebuild with LIBC_STARTUP_TRACE=min (and relink the\n'
	printf '         ports: --scope core does not touch them) before drawing a conclusion.\n'
elif [ "$n_silent" -gt 0 ]; then
	printf 'VERDICT: the fault is BEFORE libc -- exec / program loading, kernel side.\n'
elif [ "$n_libc_only" -gt 0 ]; then
	printf 'VERDICT: the fault is INSIDE a libc initialiser.\n'
else
	printf 'VERDICT: no event in this bench; the rate bound is %d/%d clean.\n' \
		"$n_ok" "$(( n_ok + n_silent + n_libc_only ))"
fi
