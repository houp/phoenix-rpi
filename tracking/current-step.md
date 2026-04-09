# Current Step

## Metadata

- Step ID: `STEP-0447`
- Title: Await the next Pi 4 board retry on the mid-register-clear split image
- Status: `in_progress`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- run the next real Pi 4 board retry on the refreshed GPIO42 telemetry image
  and determine whether the current failure is:
  - before the midpoint of generic AArch64 `_start` register clearing
  - after the midpoint but before the end of register clearing
  - or later than the new post-register-clear checkpoint

## Scope

In scope:

- flashing the refreshed Pi 4 SD image
- recording a high-framerate close-up LED video from power-on
- mapping the highest completed checkpoint group against the current `1..13`
  stage table
- using that result to choose the next smallest earliest-boot step

Out of scope:

- unrelated USB, framebuffer, shell, or later-runtime work
- broad early-boot rewrites before the narrowed telemetry result is observed

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/manual-operator-instructions.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- a new board retry is performed on the refreshed image
- the LED video is long enough to decode the slower `1..13` checkpoint map
- the highest completed checkpoint is identified with enough confidence to
  select the next bounded earliest-boot implementation step

## Validation Plan

- Hardware:
  - verify the exported image with
    `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`
  - flash the full SD image
  - record at least `70` seconds of high-framerate LED video

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-09-pi4-stage4-mid-register-clear-split.md`

## Notes

- `IMG_0005.mov` is actually `30.01 fps` according to `ffprobe`, not `60 fps`.
- The previous image still most strongly fit failure before the old stage `5`,
  so the current image adds one midpoint checkpoint inside the register-clearing
  block itself.
- The current telemetry checkpoint map is:
  - `1`: armstub primary-core entry
  - `2`: armstub after early timer / GIC preparation
  - `3`: armstub just before the fixed-address jump to `plo`
  - `4`: earliest generic AArch64 `plo` `_start`
  - `5`: midpoint of general-purpose register clearing
  - `6`: end of general-purpose register clearing
  - `7`: after `currentEL` sampling, before EL dispatch
  - `8`: `start_el3`
  - `9`: `start_el2`
  - `10`: `start_el1`
  - `11`: `start_common`
  - `12`: core-0 branch to `_startc`
  - `13`: unexpected-EL trap path
- Current refreshed exported image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `03a0729254dc0bc81f542fe8db276f7a2b70d3fb76de9fc7303ea470aca83137`
