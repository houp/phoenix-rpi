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
# ONLY MARKERS AFTER THE COMMAND ECHO COUNT. The instrument is linked into every
# binary, so a normal boot prints ~20 markers before the prompt is even reached.
# Counting markers over the whole log would score a genuinely silent launch as
# "reached _libc_init" on the strength of the boot's own markers -- i.e. it would
# confidently return the WRONG verdict, which is worse than none.
#
# A silent trial is only EVIDENCE if the window after the command was long
# enough to exclude the documented ~68 s cold-exec class (a large ELF demand-paged
# from the NFS root: see docs -- project_sdboot_largeexec_slowstart). Below that,
# "printed nothing" and "had not finished loading yet" are the same observation.
# Sweeping the artifact history for trials whose LAST output is the command echo
# finds five, across four benches, and every one ran a ~45 s window -- so the
# bench-derived reproductions are not sound evidence. That sweep is a FLOOR, not
# a census: it misses any silent launch followed by unrelated driver chatter, and
# it says nothing about the two long-window observations recorded elsewhere (the
# original event at idle 240 / max 300, and the spawn-storm stall of >=218 s),
# which stand. Trials under MIN_WINDOW_SECS are reported INCONCLUSIVE.
#
# Two more classes are essential and easy to miss:
#   * a trial with no psh prompt is a truncated capture: not evidence either way;
#   * a trial where the command was never ECHOED never launched the app at all
#     -- harness noise, not a premain event. Counting it as one is what made the
#     recorded 2-in-23 bench read as 3-in-24. Pass <cmd-echo-ERE> to separate it.
#
# Usage: ./scripts/classify-premain-trials.sh <label> [app-first-line-ERE] [cmd-echo-ERE]
#        default app marker: 'main\(\) entered'

set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
label="${1:?usage: classify-premain-trials.sh <label> [app-first-line-ERE] [cmd-echo-ERE]}"
app_re="${2:-main\(\) entered}"
cmd_re="${3:-}"
# The floor comes from the ~68 s cold-exec class plus a wide margin; 240 s is the
# figure the prior investigation settled on. Override only with a reason.
min_window="${MIN_WINDOW_SECS:-240}"

shopt -s nullglob
logs=( "${repo_root}/artifacts/rpi4b-uart/"rpi4b-uart-*-"${label}"-T*.log )
if [ ${#logs[@]} -eq 0 ]; then
	printf 'classify-premain-trials: no logs for label "%s"\n' "$label" >&2
	exit 1
fi

MARKER='libc-init: enter' APP_RE="$app_re" CMD_RE="$cmd_re" LABEL="$label" \
MIN_WINDOW="$min_window" \
python3 - "${logs[@]}" <<'PYEOF'
import os, re, sys

marker = os.environ['MARKER']
app_re = re.compile(os.environ['APP_RE'])
cmd_src = os.environ['CMD_RE']
cmd_re = re.compile(cmd_src) if cmd_src else None
label = os.environ['LABEL']

min_window = float(os.environ['MIN_WINDOW'])
counts = {'OK': 0, 'SILENT': 0, 'LIBC-INIT': 0, 'NO-CMD': 0, 'VOID': 0, 'INCONCLUSIVE': 0}
STAMP_INSERT = re.compile(r'\n\[T\+\s*[0-9.]+\] ')
WINDOW_END = re.compile(r'\*\*\* capture-window ended after ([0-9.]+)s')


def window_secs(lines, start):
    """Seconds the harness waited AFTER the command echo, or None if unknown.

    THE WINDOW CANNOT BE INFERRED FROM THE DATA IN THE LOG. `[T+ x.xx]` stamps
    mark when a chunk ARRIVED, and the whole point of a silent trial is that
    nothing arrived -- so the last stamp sits just after the command echo no
    matter how long the harness then waited. Taking max(stamp) as the window
    scores a genuine 300 s hang as INCONCLUSIVE: the same error as counting a
    45 s window as an event, only inverted, and it fires on exactly the run this
    exists to catch. File mtime is no better -- it covers the whole trial
    including boot, so it OVERSTATES, and against a MINIMUM floor overstating is
    the direction that lets a too-short window through.

    So only an explicit marker written by psh-interact.py counts. Absent it the
    window is unknown and no event is claimed; the harness's own stdout carries
    its `max-cmd-secs (Ns) reached` line for that case.
    """
    for s in lines[start:]:
        m = WINDOW_END.search(s)
        if m:
            return float(m.group(1))
    return None
traced_anywhere = False
rows = []

for path in sys.argv[1:]:
    text = open(path, 'rb').read().decode('utf8', 'replace')
    # psh-interact --stamp writes "\n[T+  x.xx] " BETWEEN chunks, which splits the
    # capture mid-token: a healthy trial's `libc-init: enter` comes out as "libc"
    # on one line and "-init: enter" on the next, and a line-wise grep then scores
    # a perfectly healthy run as having reached neither libc nor main. Undo the
    # insertion before matching -- it is exactly this prefix and nothing else, so
    # removing it restores the original byte stream.
    text = STAMP_INSERT.sub('', text)
    lines = text.split('\n')
    if any(marker in s for s in lines):
        traced_anywhere = True

    # The command echo is the boundary: everything before it belongs to the boot.
    # Use the FIRST match. The psh prompt echoes the line as it is typed, so the
    # first hit is the launch. A LAST-match boundary is wrong and quietly so:
    # quakespasm itself prints `Command line: /usr/bin/quakespasm -loadbench`
    # AFTER `main() entered`, so the boundary lands past the app's own output and
    # every healthy trial scores SILENT -- the exact inversion of the verdict.
    start = None
    if cmd_re is not None:
        for n, s in enumerate(lines):
            if cmd_re.search(s):
                start = n
                break
    tail = lines[start + 1:] if start is not None else lines

    has_prompt = any('(psh)%' in s for s in lines)
    n_marker = sum(1 for s in tail if marker in s)
    n_app = sum(1 for s in tail if app_re.search(s))

    if not has_prompt:
        cls, detail = 'VOID', 'no psh prompt -- truncated capture, not evidence'
    elif cmd_re is not None and start is None:
        cls, detail = 'NO-CMD', 'command never echoed -- never launched, not evidence'
    elif n_app > 0:
        cls, detail = 'OK', 'post-echo markers=%d app=%d' % (n_marker, n_app)
    elif n_marker > 0:
        cls, detail = 'LIBC-INIT', 'reached _libc_init (%d), never reached main()' % n_marker
    else:
        w = window_secs(lines, start if start is not None else 0)
        if w is None:
            cls = 'INCONCLUSIVE'
            detail = ('silent, but the log carries no end-of-window marker, so the wait '
                      'is unknown -- check the harness stdout for its max-cmd-secs line')
        elif w < min_window:
            cls = 'INCONCLUSIVE'
            detail = ('silent, but the window was only %.0f s (< %.0f s): a slow cold '
                      'exec looks identical' % (w, min_window))
        else:
            cls = 'SILENT'
            detail = ('0 post-echo markers in %.0f s -- never reached _libc_init' % w)
    counts[cls] += 1
    rows.append((os.path.basename(path), cls, detail))

print('%-52s %-12s %s' % ('LOG', 'CLASS', 'DETAIL'))
print('%-52s %-12s %s' % ('-' * 52, '-' * 12, '-' * 6))
for r in rows:
    print('%-52s %-12s %s' % r)

n = len(rows)
print('\n=== %s: %d trials -- OK %d | SILENT %d | LIBC-INIT %d | INCONCLUSIVE %d | NO-CMD %d | VOID %d ==='
      % (label, n, counts['OK'], counts['SILENT'], counts['LIBC-INIT'],
         counts['INCONCLUSIVE'], counts['NO-CMD'], counts['VOID']))
if counts['INCONCLUSIVE']:
    print('NOTE: %d trial(s) printed nothing but ran too short a window to distinguish a'
          % counts['INCONCLUSIVE'])
    print('      hang from a slow cold exec. Re-run those with --ready-line + a 300 s')
    print('      --max-cmd-secs (and --stamp) before counting them as events.')

if cmd_re is None:
    print('NOTE: no <cmd-echo-ERE> given, so markers were counted over the WHOLE log')
    print('      (the boot\'s own ~20 markers included) and a trial whose command never')
    print('      reached the shell is counted as SILENT. Pass one for a real verdict.')
    sys.exit(0)

if not traced_anywhere:
    print('VERDICT: none possible -- NO trial printed a marker, so this bench ran an')
    print('         UNTRACED build. Rebuild with LIBC_STARTUP_TRACE=min (and relink the')
    print('         ports: --scope core does not touch them) before drawing a conclusion.')
elif counts['SILENT'] > 0 and counts['LIBC-INIT'] > 0:
    print('VERDICT: BOTH classes occurred (%d before libc, %d inside a libc initialiser)'
          % (counts['SILENT'], counts['LIBC-INIT']))
    print('         -- two distinct faults, or one that can strike either side of the')
    print('         boundary. Do not collapse them into a single rate.')
elif counts['SILENT'] > 0:
    print('VERDICT: the fault is BEFORE libc -- exec / program loading, kernel side.')
elif counts['LIBC-INIT'] > 0:
    print('VERDICT: the fault is INSIDE a libc initialiser.')
elif counts['INCONCLUSIVE'] > 0:
    print('VERDICT: no CONFIRMED event, but %d trial(s) are inconclusive, so this bench'
          % counts['INCONCLUSIVE'])
    print('         does not bound the rate either. It is %d clean out of %d that could be'
          % (counts['OK'], counts['OK'] + counts['INCONCLUSIVE']))
    print('         scored at all -- do not quote it as %d/%d clean.'
          % (counts['OK'], counts['OK']))
else:
    print('VERDICT: no event in this bench; the rate bound is %d/%d clean.'
          % (counts['OK'], counts['OK'] + counts['SILENT'] + counts['LIBC-INIT']))
PYEOF
