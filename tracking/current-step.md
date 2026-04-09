# Current Step

## Metadata

- Step ID: `STEP-0439`
- Title: Await the next Pi 4 board retry on the earliest `plo` `_start` GPIO42 image
- Status: `pending`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- collect the next real-hardware answer after adding the first hardware-visible
  GPIO42 pattern at the earliest `plo` `_start` entry point

## Scope

In scope:

- rewriting the SD card from the refreshed verified image
- booting the real Pi 4 board
- recording:
  - final ACT LED state or sequence
  - blank-or-visible HDMI result
  - any keyboard-visible reaction

Out of scope:

- new code changes before the next board result
- unrelated runtime, USB, or framebuffer changes

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/pi4-first-hardware-trial.md`
- `docs/manual-operator-instructions.md`
- `docs/source-artifacts.md`
- `manifests/2026-04-09-pi4-earliest-plo-entry-led.md`

## Acceptance Criteria

- the next board retry reports the observed ACT LED result
- the result is paired with:
  - screen state
  - any keyboard-visible reaction
- the next implementation step can then choose between:
  - still-earlier branch or reset diagnosis
  - later `plo` entry diagnosis
  - a wider real-hardware boot-model change

## Validation Plan

- rewrite the SD card from the refreshed exported image
- boot the real Pi 4 and observe ACT LED, screen, and keyboard behavior

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-09-pi4-earliest-plo-entry-led.md`

## Notes

- The latest real Pi 4 board result on the fixed-address armstub image was:
  - both LEDs on at power-up
  - green off
  - green briefly on again
  - green off again
  - green on later and then steady on
  - blank screen
  - no keyboard-visible reaction
- That changed sequence strongly suggests the fixed-address branch altered the
  early hardware behavior but still did not prove that `plo` itself executed.
- The current image keeps the fixed-address armstub handoff and now adds a
  Pi-4-only GPIO42 pulse pattern at the very top of generic AArch64 `plo`
  `_start`, before register clearing and exception-level setup.
