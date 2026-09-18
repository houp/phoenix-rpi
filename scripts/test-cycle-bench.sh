#!/usr/bin/env bash
#
# test-cycle-bench.sh — run N back-to-back netboot cycles and summarize
# the boot-stage pass rate. Useful for measuring flakiness in subsystems
# that are known to be silicon- or boot-timing-sensitive (VL805 USB,
# BCM43455 WiFi).
#
# Usage:
#   ./scripts/test-cycle-bench.sh <N> <label> [--capture-secs <s>] [-- <cmd>...]
#
# Examples:
#   ./scripts/test-cycle-bench.sh 5 vl805-baseline
#   ./scripts/test-cycle-bench.sh 10 dhcp-trial --capture-secs 240
#   ./scripts/test-cycle-bench.sh 8 unixsock -- "/bin/test-libc-unix-socket -v"
#   # a GPU game needs a longer window AND a readiness marker, or a slow start
#   # scores INCONCLUSIVE (which reads like a failure until you open the log):
#   ./scripts/test-cycle-bench.sh 4 vkq --idle-secs 175 --max-cmd-secs 200 \
#       --ready-line 'present 30' --ready-extra-secs 90 -- "vkquake +map start"
#
# With `-- <cmd>...` each trial boots to the psh prompt and runs those commands
# (via test-cycle-psh-interact.sh) instead of just capturing a boot, and the
# summary additionally counts Unity results per trial. That is what an
# intermittent USERSPACE bug needs: a boot-only bench cannot tell a run where
# the test passed from one where it never executed, which is exactly the
# A/B/C classing mistake recorded in the W36 log.
#
# Each trial is labeled "<label>-T<i>" and logged under
# artifacts/rpi4b-uart/. After all trials run, uart-summary.sh is
# called on each log and pass/fail counts are aggregated across the
# canonical STAGES (psh prompt, lwip started, etc.).
#
# Exits 0 if all trials complete (regardless of subsystem outcome).

set -u
set -o pipefail

if [ $# -lt 2 ]; then
    echo "usage: test-cycle-bench.sh <N> <label> [--capture-secs <s>] [--idle-secs <s>] [--wait-secs <s>]" >&2
    echo "       [--max-cmd-secs <s>] [--ready-line <ERE>] [--ready-extra-secs <s>] [--stamp]" >&2
    echo "       [-- <cmd>...]" >&2
    exit 1
fi

N="$1"
label="$2"
shift 2

# Per-trial window. The old hardcoded 60/150 was not enough for a GPU game after
# the 2026-09-04 vkQuake sync (upstream's shader-module count went 34 -> 67, so
# first frame arrives later): trials ended mid-load and scored INCONCLUSIVE, which
# is indistinguishable from a failure unless you read the log. --ready-line makes
# max-cmd-secs the deadline for REACHING readiness rather than the whole budget.
idle_secs=60
max_cmd_secs=150
# Deadline for REACHING the psh prompt. test-cycle-psh-interact.sh defaults to 150,
# which the first cycle of a session can miss -- it boots slower, and the trial then
# scores "A (no shell)", indistinguishable from a real boot failure. The showcase
# gate raised its own copy to 220 for exactly this reason.
wait_secs_arg=()
ready_args=()
stamp_arg=()
capture_secs_arg=()
# A LOOP, not an if/elif chain: the original consumed at most ONE option, so a
# second flag would have been silently swallowed into the command list.
while [ $# -ge 2 ]; do
    case "$1" in
    --capture-secs)     capture_secs_arg=( --capture-secs "$2" ); shift 2 ;;
    --idle-secs)        idle_secs="$2"; shift 2 ;;
    --wait-secs)        wait_secs_arg=( --wait-secs "$2" ); shift 2 ;;
    --max-cmd-secs)     max_cmd_secs="$2"; shift 2 ;;
    --ready-line)       ready_args+=( --ready-line "$2" ); shift 2 ;;
    --ready-extra-secs) ready_args+=( --ready-extra-secs "$2" ); shift 2 ;;
    # --stamp takes no argument, so it must be handled here AND in the $#-eq-1
    # sweep below -- the loop above only runs while two args remain, so a trailing
    # `--stamp --` would otherwise fall through to the command list.
    --stamp)            stamp_arg=( --stamp ); shift ;;
    # Bench the SD CARD instead of the netboot tree. dnsmasq must already be DOWN
    # (scripts/netboot-server-down.sh) so the firmware falls back to the card, and
    # must be brought back up afterwards. A command-less trial then uses
    # test-cycle-netboot.sh --sd-boot, which also zeroes the DHCP watchdog --
    # without that the watchdog's bridge recovery restarts dnsmasq mid-bench and
    # later trials quietly netboot instead, which would not show up in the table.
    --sd-boot)          sd_boot=1; shift ;;
    # The bare `--` separator ends option parsing -- it must be matched BEFORE the
    # --* catch-all below, or the catch-all rejects the separator itself.
    --)                 break ;;
    # An UNKNOWN --flag must be fatal. Breaking out of the loop leaves it sitting
    # where `--` is expected, so the command list below comes out EMPTY and every
    # trial silently degrades to a plain boot capture -- 20 trials that look like
    # they ran and sent nothing. That cost two full runs when --inter-cmd-secs
    # (a test-cycle-psh-interact.sh option, not one of ours) was passed here.
    --*)
        printf 'test-cycle-bench.sh: unknown option %s\n' "$1" >&2
        printf '  known: --capture-secs --idle-secs --wait-secs --max-cmd-secs --ready-line --ready-extra-secs --stamp\n' >&2
        printf '  NOTE: --inter-cmd-secs belongs to test-cycle-psh-interact.sh and is not forwarded.\n' >&2
        exit 2
        ;;
    *)                  break ;;
    esac
done

# A no-argument flag can be the last thing before `--`, which the two-argument
# loop above cannot see. Sweep those here.
while [ $# -ge 1 ] && [ "$1" = "--stamp" ]; do
    stamp_arg=( --stamp ); shift
done

# Everything after `--` is a psh command line to run in each trial.
cmds=()
if [ $# -ge 1 ] && [ "$1" = "--" ]; then
    shift
    cmds=( "$@" )
    set --
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

if ! [[ "$N" =~ ^[1-9][0-9]*$ ]]; then
    echo "test-cycle-bench: N must be a positive integer, got '$N'" >&2
    exit 1
fi

# A label reused from an earlier bench leaves BOTH runs matching the same
# `*-<label>-T*.log` glob, so any later `grep artifacts/.../*<label>*` silently
# mixes trials from different builds -- and the mixture reads as one clean run.
# (2026-09-18: `stkguard` collided with a 2026-09-10 bench; three stale trials
# would have been counted as part of a twelve-trial soak.) Warn, do not refuse:
# re-running a label deliberately is legitimate, the reader just has to know.
prior=$(ls -1 "${repo_root}/artifacts/rpi4b-uart"/rpi4b-uart-*-"${label}"-T*.log 2>/dev/null | wc -l)
if [ "$prior" -gt 0 ]; then
    printf '\n!! LABEL REUSED: %d existing log(s) already match "%s-T*".\n' "$prior" "$label" >&2
    printf '!! Scope every later grep by date (rpi4b-uart-%s-*-%s-T*.log) or these\n' \
        "$(date +%Y%m%d)" "$label" >&2
    printf '!! trials will be mixed with the older run.\n\n' >&2
fi

printf '=== bench start: %d trials, label="%s" ===\n' "$N" "$label"

sd_flag=""
sd_boot_flag=""
if [ "${sd_boot:-0}" = 1 ]; then
    sd_flag="--skip-server-up"
    sd_boot_flag="--sd-boot"
    printf 'lane: SD CARD (dnsmasq must be DOWN; restore it afterwards)\n'
fi

logs=()
for i in $(seq 1 "$N"); do
    trial_label="${label}-T${i}"
    printf '\n=== trial %d/%d (%s) ===\n' "$i" "$N" "$trial_label"
    if [ "${#cmds[@]}" -gt 0 ]; then
        "${repo_root}/scripts/test-cycle-psh-interact.sh" ${sd_flag} --label "$trial_label" \
            --inter-cmd-secs 8 --idle-secs "$idle_secs" --max-cmd-secs "$max_cmd_secs" \
            "${wait_secs_arg[@]}" \
            "${ready_args[@]}" "${stamp_arg[@]}" -- "${cmds[@]}" || true
    else
        "${repo_root}/scripts/test-cycle-netboot.sh" ${sd_boot_flag} --label "$trial_label" "${capture_secs_arg[@]}" || true
    fi
    log=$(ls -t "${repo_root}/artifacts/rpi4b-uart"/rpi4b-uart-*-"$trial_label".log 2>/dev/null | head -n 1 || true)
    if [ -n "$log" ]; then
        logs+=( "$log" )
    fi
done

printf '\n\n=== BENCH SUMMARY: label="%s" trials=%d logs=%d ===\n' "$label" "$N" "${#logs[@]}"

# When commands were run, classify each trial the way the W36 counting rule
# requires: a run only counts as a TEST RESULT if the log has a psh prompt AND
# at least one "TEST(" line. Anything else is a boot/exec failure, not evidence.
if [ "${#cmds[@]}" -gt 0 ]; then
    printf '\n--- per-trial test results (class C only counts) ---\n'
    for log in "${logs[@]}"; do
        # NB: `grep -c` PRINTS 0 and EXITS 1 when there are no matches, so a
        # `|| echo 0` fallback appends a second line and the count becomes
        # "0\n0" -- which makes [ "$x" -eq 0 ] fail and silently misclassify a
        # class-B run (no output) as class C (ran). Use `|| true`.
        prompt=$(grep -ac '(psh)%' "$log" || true)
        ran=$(grep -ac 'TEST(' "$log" || true)
        summary=$(grep -aoE '[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored' "$log" 2>/dev/null | tail -n 1)
        # A trial whose capture ended at (or just after) the command produced NO
        # EVIDENCE -- it must not be counted as a failure. Without this, a bench
        # of a non-test command (a desktop launch, a game) reports every trial as
        # "B (no output)" and a truncated capture is indistinguishable from a real
        # failure. That exact confusion has produced false conclusions here before.
        if ! "$script_dir/check-capture-complete.py" --quiet "$log" \
                --commands "${cmds[@]}" >/dev/null 2>&1; then
            cls="VOID (capture truncated)"
        elif [ "$prompt" -eq 0 ]; then
            cls="A (no shell)"
        elif [ "$ran" -eq 0 ]; then
            cls="B (ran, no test output)"
        else
            cls="C (ran)"
        fi
        printf '  %-58s %-14s %s\n' "$(basename "$log")" "$cls" "${summary:-<no summary>}"
    done
fi

if [ "${#logs[@]}" -eq 0 ]; then
    printf 'no logs produced — bench infrastructure broken\n' >&2
    exit 1
fi

# Per-stage pass rate across all logs.
declare -A pass

# Stages to track — match check_stage labels in uart-summary.sh.
stages=(
    "kernel banner"
    "fbcon up"
    "psh prompt"
    "lwip started"
    "genet link up"
    "netif has IP"
)

for log in "${logs[@]}"; do
    out=$("${repo_root}/scripts/uart-summary.sh" "$log" 2>/dev/null || true)
    for stage in "${stages[@]}"; do
        if printf '%s\n' "$out" | grep -q "\[YES\] ${stage}"; then
            pass["$stage"]=$(( ${pass["$stage"]:-0} + 1 ))
        fi
    done
done

printf '%-22s %s\n' "STAGE" "PASS / $N"
printf '%-22s %s\n' "----------------------" "-------"
for stage in "${stages[@]}"; do
    n=${pass["$stage"]:-0}
    printf '  %-20s %d\n' "$stage" "$n"
done

echo
echo "logs:"
for log in "${logs[@]}"; do
    printf '  %s\n' "$log"
done
