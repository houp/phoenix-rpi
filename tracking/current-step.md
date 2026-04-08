# Current Step

## Metadata

- Step ID: `STEP-0429`
- Title: Retry the Pi 4 board boot on the FAT-verified exported image
- Status: `in_progress`
- Date: `2026-04-08`
- Milestone / phase: `Phase 1`

## Objective

- re-run the next real Pi 4 board boot using the now-correct host-visible SD
  image whose FAT boot partition has been verified on macOS

## Scope

In scope:

- flashing the corrected Pi 4 SD image to the real board's microSD card
- observing the first board result on the corrected exported image
- classifying the result and using it to choose the next bounded boot step

Out of scope:

- unrelated image-export or host-artifact work
- wide bring-up changes before the board result is classified

## Expected Repositories

- none unless the board result uncovers the next implementation step

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `docs/pi4-first-hardware-trial.md`
- `docs/manual-operator-instructions.md`

## Acceptance Criteria

- the corrected image is flashed to the card without using the partition node
- the board result is captured in enough detail to classify:
  - screen state
  - ACT LED state
  - keyboard-visible reaction
- the next smallest implementation step is chosen from that evidence

## Validation Plan

- run `scripts/verify-rpi4b-sdimg.sh`
- flash the corrected image to the whole SD-card device
- boot the Pi 4 and record the observed result with
  `docs/pi4-first-hardware-trial.md`

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-08-pi4-sdimg-export-fix.md`

## Notes

- The host-side SD-image export corruption is now fixed.
- The current exported image SHA-256 is:
  `44085197192f5578759269813c3aa38a8adcf04b18bc0092ec509b8fa5543920`.
