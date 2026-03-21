# Current Step

## Metadata

- Step ID: `STEP-0281`
- Title: Implement the first macOS flashing-workflow runbook step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest operator-facing runbook update that makes the current Pi 4
  SD-card image usable for a first manual hardware trial

## Scope

In scope:

- add one explicit macOS flashing procedure for `rpi4b-sd.img`
- document the exact current artifact path and checksum
- document the current no-UART expectations for the first Pi 4 boot attempt
- keep the step no-hardware and documentation-only

Out of scope:

- running the flashing procedure
- changing Phoenix source code
- real hardware execution

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `artifacts/rpi4b/rpi4b-sd.img`
- `docs/manual-operator-instructions.md`
- `docs/testing-automation.md`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the operator runbook contains one explicit macOS flashing workflow
- the no-UART expectations are documented clearly enough that the first manual
  boot attempt is not overinterpreted
- no Phoenix upstream repo changes are introduced

## Validation Plan

- review the current artifact path and checksum
- confirm the runbook now covers both flashing and the current first-boot
  observability limits
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-macos-flash-doc-scope.md`

## Notes

- Risks:
  avoid widening into SD-writing automation, real-device execution, or board
  bring-up changes
- Dependencies:
  completed `STEP-0280` flashing-workflow documentation scoping
- User-visible control point before next step:
  after this documentation step lands, the next bounded move can shift back to
  technical work on alternate observability for a no-UART Pi 4 lab
