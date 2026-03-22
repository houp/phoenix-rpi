# Current Step

## Metadata

- Step ID: `STEP-0405`
- Title: Re-hold the project at the Pi 4 hardware boundary
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- keep the project explicitly stopped at the real Pi 4 board boundary after the
  final useful operator-side helper is in place

## Scope

In scope:

- preserving the final handoff set
- making the hardware dependency explicit again
- preventing drift back into speculative pre-hardware work

Out of scope:

- manual hardware execution itself
- new code-side USB or xHCI feature work until board evidence exists
- automatic flashing or other destructive helpers

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the final handoff set is preserved
- the current dependency is explicitly the first board run
- no further pre-hardware work is implied

## Validation Plan

- review the current helper set and confirm nothing stronger remains than the
  actual board boot

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-report-helper.md`

## Notes

- Risks:
  avoid widening this into more pre-hardware busywork
- Dependencies:
  completed `STEP-0404` report-helper implementation
- User-visible control point before next step:
  after this step, the next bounded move should be the user's first board boot
  result or filled report file
