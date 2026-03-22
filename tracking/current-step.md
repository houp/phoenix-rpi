# Current Step

## Metadata

- Step ID: `STEP-0400`
- Title: Scope the next step after the first real Pi 4 board result
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- hold the project at the real-hardware boundary and make the next dependency
  explicit: the first real Pi 4 board result

## Scope

In scope:

- explicitly identifying the next dependency as hardware execution
- keeping the project on-rails until board evidence exists
- documenting that further pre-hardware work should stop here unless a new
  operator-side blocker appears

Out of scope:

- manual hardware execution itself
- new code-side USB or xHCI feature work until board evidence exists
- wider packaging such as `phoenix-rtos-ports`

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

- the project state makes the real-hardware dependency explicit
- no further speculative pre-hardware work is implied
- the next bounded move is the user's board result

## Validation Plan

- review the current exported artifact and first-trial document set
- confirm that the next stronger lane is now strictly manual hardware execution

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-first-trial-classification-aid.md`

## Notes

- Risks:
  avoid widening the next move into runtime source changes before the first
  hardware result
- Dependencies:
  completed `STEP-0399` classification-aid implementation
- User-visible control point before next step:
  after this step, the next bounded move should be the first manual board boot
  result or a newly discovered operator-side blocker
