# Current Step

## Metadata

- Step ID: `STEP-0401`
- Title: Implement the Pi 4 SD-image verification and macOS flash-command helpers
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest remaining operator-side helpers that reduce flashing
  mistakes before the first real Pi 4 board run

## Scope

In scope:

- one helper to verify the exported SD-image path, size, and checksum
- one helper to print the exact macOS flashing commands for a chosen target disk
- runbook updates for those helpers

Out of scope:

- executing a destructive flash automatically
- manual hardware execution itself
- new code-side USB or xHCI feature work until board evidence exists

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`
- `scripts/`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- one non-destructive verification helper exists for the current SD image
- one non-destructive macOS flash-command helper exists
- the runbook points at both helpers

## Validation Plan

- run the verification helper on the current exported image
- run the flash-command helper in print mode with an example disk identifier
- inspect the updated runbook references

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-flash-helper-scope.md`

## Notes

- Risks:
  avoid widening the step into an automatic flashing tool
- Dependencies:
  completed `STEP-0400` flash-helper scope
- User-visible control point before next step:
  after this step, the next bounded move should be the first manual board boot
  result unless a new operator-side blocker is discovered
