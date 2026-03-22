# STEP-0357

## Title

Scope the smallest cleanup step for stale Pi 4 shell-smoke probe noise

## Date

2026-03-22

## Outcome

The next bounded step is now fixed:

- remove the stale kernel-side `create_dev` trace helpers that were added for
  an earlier false hypothesis
- keep the cleanup limited to probe removal
- use the existing generic and Pi 4 shell-smoke helpers as the validation gate

## Why This Step

After `STEP-0356`, the Pi 4 runtime is functionally back to:

- `psh: tty ready`
- `(psh)%`
- `Available commands:`

The remaining automated-smoke problem is not a missing prompt anymore. The log
is still polluted by repeated `create_dev: lookup /dev` lines emitted from old
kernel probes in `syscalls.c`, which now obscure the helper’s clean prompt
round-trip.

Those probes were diagnostic-only and should be removed under the repository’s
cleanup rule for false-hypothesis instrumentation.

## Next Step

- implement the smallest cleanup step for the stale `create_dev` probes
