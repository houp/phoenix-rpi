# Current Step

## Metadata

- Step ID: `STEP-0234`
- Title: Scope the smallest below-stdio `psh` process-entry visibility step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest non-shell-stdio visibility step after the negative
  cross-lane `psh` marker result

## Scope

In scope:

- review the current `psh` startup path in `psh.c` and `pshapp.c`
- inspect the smallest kernel-side hooks below shell-visible stdio
- choose one exact hook that can prove whether the spawned `psh` process ever
  reaches first user execution
- document the selected next implementation step

Out of scope:

- changing behavior
- real hardware work
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- likely `sources/phoenix-rtos-kernel/proc/threads.c`
- likely `sources/phoenix-rtos-kernel/proc/process.c`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the selected next patch uses one exact below-stdio hook only
- the scope names the source file and the specific condition to trace
- the result narrows the next move to one concrete implementation step

## Validation Plan

- Analysis only:
  - inspect scheduler and exec paths for the first user-mode entry point
  - prefer a one-time `psh`-specific first-schedule or first-exec marker
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-psh-startup-visibility.md`

## Notes

- Risks:
  do not widen into broad scheduler tracing when one `psh`-specific hook should
  be enough
- Dependencies:
  completed `STEP-0233` `psh` startup visibility
- Source reminder:
  neither lane shows any `psh:` marker at all, so shell-visible stdio is not
  yet a trustworthy boundary marker
- User-visible control point before next step:
  after this scope step lands, the next patch should add one kernel-side marker
  only and should avoid reopening earlier boot paths
