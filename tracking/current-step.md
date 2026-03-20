# Current Step

## Metadata

- Step ID: `STEP-0236`
- Title: Scope the smallest kernel-side `psh` root-lookup success trace
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest next hook after first user execution that can prove
  whether `psh` gets past its `lookup("/")` wait loop

## Scope

In scope:

- review the current `psh` startup path in `psh.c` and `pshapp.c`
- inspect the earliest `psh`-specific syscall-side hook after first execution
- prefer a one-time root-lookup success trace over a broader open-path trace
- document the exact next implementation step

Out of scope:

- changing behavior
- broad syscall tracing
- real hardware work
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- likely `sources/phoenix-rtos-kernel/syscalls.c`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the selected next patch traces one exact `psh` root-lookup success condition
- the scope names the source file and the exact trigger condition
- the result narrows the next move to one concrete implementation step

## Validation Plan

- Analysis only:
  - inspect `syscalls_lookup()` and related path-resolution helpers
  - choose the smallest `psh`-filtered `lookup("/")` success marker
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-psh-first-user-schedule.md`

## Notes

- Risks:
  avoid widening into generic pathname tracing when the next question is only
  whether `psh` sees `/`
- Dependencies:
  completed `STEP-0235` `psh` first-user-schedule visibility
- Source reminder:
  both lanes now prove `psh` reaches user mode, so the next split should move
  to the earliest `psh`-specific syscall result
- User-visible control point before next step:
  after this scope step lands, the next patch should add one `psh`-filtered
  root-lookup marker only
