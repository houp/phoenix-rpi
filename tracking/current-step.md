# Current Step

## Metadata

- Step ID: `STEP-0448`
- Title: Split the Pi 4 failure between end-of-clear and post-`currentEL` entry
- Status: `in_progress`
- Date: `2026-04-09`
- Milestone / phase: `Phase 1`

## Objective

- add the next narrowest GPIO42 split immediately around `currentEL` sampling in
  earliest generic AArch64 `plo _start`
- determine whether the current real Pi 4 failure is:
  - before the `mrs currentEL` instruction
  - after `currentEL` sampling but before the existing stage `7` marker
  - or inside the first EL-dispatch path selected after `currentEL`

## Scope

In scope:

- adding one tighter bounded checkpoint split around `mrs currentEL`
- rebuilding the Pi 4 A72 image
- re-exporting and verifying the Pi 4 SD image
- updating the hardware runbook and tracker for the next board retry

Out of scope:

- unrelated USB, framebuffer, shell, or later-runtime work
- broad early-boot rewrites before the `currentEL` boundary is split

## Expected Repositories

- `sources/plo`
- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/sources/plo/hal/aarch64/generic/_init.S`
- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- the earliest generic AArch64 `plo _start` path has one more bounded split
  around `currentEL`
- the Pi 4 A72 image rebuilds successfully
- the exported SD image is refreshed and passes the FAT-aware verifier
- the next board retry can distinguish pre-`currentEL` failure from
  post-`currentEL` but pre-EL-dispatch failure

## Validation Plan

- Build and export:
  - rebuild `aarch64a72-generic-rpi4b`
  - reassemble bootfs and SD image
  - export the image through
    `/Users/witoldbolt/phoenix-rpi/scripts/export-rpi4b-sdimg.sh`
  - verify the exported image with
    `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-09-pi4-stage4-mid-register-clear-split.md`

## Notes

- `IMG_0009.mov` is `59.93 fps` according to `ffprobe`.
- The strongest current interpretation is:
  - the later ACT pulse envelopes fit completion through stage `6`
  - no later visible group fits the existing stage `7` marker
  - so the next useful split is around `mrs currentEL`, not deeper EL-path code
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
