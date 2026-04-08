# Current Step

## Metadata

- Step ID: `STEP-0412`
- Title: Await the next Pi 4 board retry on the rebuilt firmware-placement image
- Status: `in_progress`
- Date: `2026-04-08`
- Milestone / phase: `Phase 1`

## Objective

- run the next real Pi 4 board trial on the rebuilt image that aligns the Pi 4
  A72 loader placement with the firmware default 64-bit load model, then
  classify the result before touching wider bring-up code

## Scope

In scope:

- flashing the rebuilt image
- booting the real Raspberry Pi 4 again
- classifying the observed result with the focused first-trial checklist
- using that board evidence to choose the smallest next implementation step

Out of scope:

- speculative code changes before the rebuilt image is tested on hardware
- wider Pi 4 bring-up changes unrelated to the observed next failure
- unrelated refactors

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/pi4-first-hardware-trial.md`
- `docs/manual-operator-instructions.md`
- `tracking/current-step.md`
- future trial report or chat evidence

## Acceptance Criteria

- the rebuilt image is flashed to the board
- the next Pi 4 board result is captured clearly enough to classify
- the next implementation step is chosen from observed evidence, not guesswork

## Validation Plan

- verify the exported image with:
  - `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`
- flash and run the board
- classify the result using:
  - `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-08-pi4-firmware-placement-rebuild.md`

## Notes

- Risks:
  without UART, the next board result still has limited observability, so the
  visible HDMI state and gross LED behavior must be recorded carefully
- Dependencies:
  rebuilt Pi 4 image with:
  - `plo` linked at `0x00200000`
  - no forced Pi 4 `kernel_address`
  - no forced `boot_load_flags`
- User-visible control point before next step:
  after the board retry, stop and classify the observed result before taking
  wider action
