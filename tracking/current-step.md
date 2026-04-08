# Current Step

## Metadata

- Step ID: `STEP-0437`
- Title: Await the next Pi 4 board retry on the fixed-address armstub image
- Status: `pending`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- collect the next real-hardware answer after replacing the firmware-patched
  `kernel_entry32` handoff with a direct branch to the known configured
  Pi 4 `kernel_address`

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
- `manifests/2026-04-09-pi4-fixed-armstub-entry.md`

## Acceptance Criteria

- the next board retry reports the observed ACT LED result
- the result is paired with:
  - screen state
  - any keyboard-visible reaction
- the next implementation step can then choose between:
  - earlier fixed-address armstub diagnosis
  - first post-branch `plo` entry diagnosis
  - a wider real-hardware boot-model change

## Validation Plan

- rewrite the SD card from the refreshed exported image
- boot the real Pi 4 and observe ACT LED, screen, and keyboard behavior

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-09-pi4-fixed-armstub-entry.md`

## Notes

- The last real Pi 4 board result on the pre-kernel-branch image was:
  - ACT LED on
  - brief off pulse
  - then on forever
  - blank screen
  - no keyboard-visible reaction
- The current image keeps the same GPIO42 split but now jumps directly to
  `0x40080000` instead of using firmware-patched `kernel_entry32`.
- The next board retry is expected to answer whether the previous loop was
  caused by the raw-`kernel8.img` versus firmware-entry mismatch.
