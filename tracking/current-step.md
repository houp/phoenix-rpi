# Current Step

## Metadata

- Step ID: `STEP-0115`
- Title: Define the first bounded emulated Pi 4 boot blocker after the no-output `raspi4b` smoke
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- choose the smallest next diagnostic or code step that can turn the current no-output `raspi4b` smoke into observable early Pi 4 boot progress

## Scope

In scope:

- inspect the current Pi 4 QEMU smoke command, artifact set, and QEMU board expectations
- bound the most likely earliest blocker into one small next step
- prefer the smallest explanation or diagnostic that can be validated quickly in QEMU `raspi4b`
- update manifests and tracking with the selected next blocker

Out of scope:

- broad Pi 4 peripheral-debug work
- broad refactors across `plo`, kernel, and project targets in one step
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- changing the existing generic `virt` lane

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- relevant Pi 4 build artifacts and QEMU invocation notes
- manifests and tracking updates for this environment step

## Acceptance Criteria

- the most likely next emulated Pi 4 boot blocker is stated explicitly
- the blocker is small enough to become one bounded implementation or diagnostic step
- the blocker choice is backed by current QEMU smoke evidence rather than vague speculation

## Validation Plan

- Review:
  inspect the current Pi 4 smoke evidence, QEMU board constraints, and artifact layout
- Build:
  not required
- Emulator:
  use the already recorded `raspi4b` smoke result from `STEP-0114`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-qemu-refresh.md`

## Notes

- Risks:
  do not let this planning step widen into a broad debugging session without first selecting one tight blocker
- Dependencies:
  completed `STEP-0114`
- User-visible control point before next step:
  after this step lands, the next bounded move should be one explicit small implementation or diagnostic step aimed at the chosen first emulated Pi 4 boot blocker
