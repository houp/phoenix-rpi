# Current Step

## Metadata

- Step ID: `STEP-0445`
- Title: Await the next Pi 4 board retry on the post-stage-4 EL-dispatch split image
- Status: `in_progress`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- run the next real Pi 4 board retry on the narrowed GPIO42 telemetry image
  and determine whether the current failure is:
  - before `currentEL` sampling
  - after `currentEL` sampling but before EL-path selection
  - inside `start_el3`, `start_el2`, or `start_el1`
  - after EL-path selection in `start_common` or at the core-0 branch

## Scope

In scope:

- flashing the refreshed Pi 4 SD image
- recording a high-framerate close-up LED video from power-on
- mapping the highest completed checkpoint group against the current `1..12`
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
- the LED video is long enough to decode the slower `1..12` checkpoint map
- the highest completed checkpoint is identified with enough confidence to
  select the next bounded earliest-boot implementation step

## Validation Plan

- Hardware:
  - verify the exported image with
    `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`
  - flash the full SD image
  - record at least `70` seconds of `60 fps` or better LED video

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-09-pi4-post-stage4-el-dispatch-split.md`

## Notes

- The `IMG_0004.mov` analysis strongly suggests the highest completed
  checkpoint on the previous image was still `4`, not `6`, because LED activity
  stopped at about `16.85s`, which fits the cumulative slower-protocol duration
  through stage `4` much better than through stage `6`.
- The current telemetry checkpoint map is:
  - `1`: armstub primary-core entry
  - `2`: armstub after early timer / GIC preparation
  - `3`: armstub just before the fixed-address jump to `plo`
  - `4`: earliest generic AArch64 `plo` `_start`
  - `5`: after general-purpose register clearing
  - `6`: after `currentEL` sampling, before EL dispatch
  - `7`: `start_el3`
  - `8`: `start_el2`
  - `9`: `start_el1`
  - `10`: `start_common`
  - `11`: core-0 branch to `_startc`
  - `12`: unexpected-EL trap path
- Current timing target:
  - about `0.4s` LED on per pulse
  - about `0.4s` LED off between pulses inside one group
  - about `2.0s` LED off between groups
- Current refreshed exported image:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- Current validated SHA-256:
  `d1e0fd5b2e3817d4e0d2ad339b63be34fb96d17f2d8a05d4e318d52a02952c20`
