# Current Step

## Metadata

- Step ID: `STEP-0427`
- Title: Await the next Pi 4 board retry on the GPIO42-proof image
- Status: `in_progress`
- Date: `2026-04-08`
- Milestone / phase: `Phase 1`

## Objective

- hold the project at the next real-hardware boundary after exporting the Pi 4
  image that drives GPIO42 from the custom armstub

## Scope

In scope:

- the first Pi 4 board retry on the refreshed SD image
- classifying the result from:
  - ACT LED behavior
  - HDMI behavior
  - keyboard-visible behavior
- preserving the current narrow hardware-feedback loop

Out of scope:

- new source changes before the next board observation arrives
- wider redesigns not driven by the next hardware result

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `docs/status.md`
- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`

## Acceptance Criteria

- the refreshed SD image is available to the operator
- the operator has a clear expectation for the GPIO42 ACT LED proof
- the next hardware outcome can be classified into a bounded next step

## Validation Plan

- board result expected from the operator should cover:
  - whether the ACT LED turns on and stays on after the initial firmware blink
  - whether the screen stays black or shows a later Phoenix transition
  - whether any keyboard-visible behavior appears
- record the next board result in chat and, if needed, in the first-trial
  report template

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-08-pi4-gpio42-armstub-proof.md`

## Notes

- The active board artifact is now the GPIO42-proof image, not the earlier
  armstub-only EL3/GIC-prep image.
- The next useful branch in diagnosis is:
  - ACT LED on: custom armstub executed; failure is later
  - ACT LED still off: failure is before or inside the current earliest custom
    armstub path
