# Current Step

## Metadata

- Step ID: `STEP-0443`
- Title: Await the next Pi 4 board retry on the slower structured LED telemetry image
- Status: `pending`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- collect the next real Pi 4 hardware answer from the slower GPIO42 telemetry
  image so the highest completed checkpoint can be identified reliably from one
  ordinary high-framerate phone video

## Scope

In scope:

- rewriting the SD card from the refreshed verified telemetry image
- booting the real Pi 4 board
- recording a high-quality ACT-LED video from power-on through the longer
  slower-telemetry window
- recording:
  - final ACT LED state and pulse groups
  - blank-or-visible HDMI result
  - any keyboard-visible reaction

Out of scope:

- new code changes before the next board result
- unrelated USB, framebuffer, or shell-runtime changes

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/status.md`
- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`
- `docs/testing-automation.md`
- `manifests/2026-04-09-pi4-led-telemetry-slower-protocol.md`

## Acceptance Criteria

- the next board retry reports the observed ACT LED pulse groups from the
  slower telemetry image
- the result is paired with:
  - screen state
  - any keyboard-visible reaction
  - whether the board appears to hang or reset
- the next implementation step can choose the exact highest completed
  checkpoint instead of guessing from a too-dense clip

## Validation Plan

- rewrite the SD card from the refreshed exported image
- boot the real Pi 4 and record a `60 fps` or better close-up of the LEDs
- keep recording for at least `70` seconds and preferably `90` seconds
- classify the highest completed telemetry group before making any new code
  change

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-04-09-pi4-led-telemetry-slower-protocol.md`

## Notes

- The current telemetry checkpoint map is:
  - `1`: armstub primary-core entry
  - `2`: armstub after early timer / GIC preparation
  - `3`: armstub just before the fixed-address jump to `plo`
  - `4`: earliest generic AArch64 `plo` `_start`
  - `5`: `plo` EL3 path selected
  - `6`: `plo` EL2 path selected
  - `7`: `plo` EL1 path selected
  - `8`: `plo` `start_common`
  - `9`: `plo` core-0 branch to `_startc`
- Current timing target:
  - about `0.4s` LED on per pulse
  - about `0.4s` LED off between pulses inside one group
  - about `2.0s` LED off between groups
- The current refreshed exported image is:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `4698611f2231fd5508e6eddeed25a24147701ce3efc209371425ea75d502f23e`
