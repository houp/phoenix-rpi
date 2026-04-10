# Current Step

## Metadata

- Step ID: `STEP-0461`
- Title: Await the next Pi 4 board retry on the first-read focus image
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- collect the next board retry on the rebuilt first-read focus image, which
  now duplicates the seam stages, inserts extra gap time, and temporarily
  micro-splits the first and second signature reads with stages `21` and `22`

## Scope

In scope:

- decode the next board video against the first-read focus map
- classify whether the live fault is:
  - before the first read
  - on the first read itself
  - between the first and second reads
  - on the second read itself
  - later in the signature-compare band

Out of scope:

- unrelated EL-path, framebuffer, DTB, or USB work
- redesigning the whole Pi 4 boot model before the dense armstub map is tested

## Expected Repositories

- `phoenix-rtos-project`
- `plo`
- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `/Users/witoldbolt/phoenix-rpi/scripts/rpi4_actled_probe_layout.py`
- `/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`
- `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`

## Acceptance Criteria

- the refreshed first-read focus image is rebuilt, exported, and FAT-verified
- the next board video can distinguish whether the live fault is:
  - before the first fixed-target read
  - on the first fixed-target read
  - between the first and second fixed-target reads
  - on the second fixed-target read
  - or later in the compare / branch band

## Validation Plan

- board retry plus LED decode
- use the new duplicated-focus protocol to reduce missed or ambiguous seam
  stages

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-led-analysis-toolchain.md`

## Notes

- the new first-read focus image now keeps the dense seam stages but adds:
  - duplicated focus-stage emission with an extra long inter-stage gap
  - `21`: immediately before the first signature-word read
  - `22`: immediately before the second signature-word read
  - barriers before both reads
- the current exported SD-image SHA-256 is now:
  - `6932d3a31fc0fee1494295c4e9d0587c689b7cde20a6fb1907d86164e9815883`
