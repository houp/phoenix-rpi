# Current Step

## Metadata

- Step ID: `STEP-0457`
- Title: Await the next Pi 4 board retry on the fixed-target-signature image
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- get the next real Pi 4 LED video on the new image that distinguishes:
  - stage `3` only: failure before or during target-signature verification
  - stage `31`: signature missing at `0x40080000`
  - stage `4`: signature present before branch
  - stage `5`: fixed-entry veneer actually entered
- preserve the new LED-analysis toolchain as the default readout path for the
  next board retry

## Scope

In scope:

- collect the next board retry on the fixed-target-signature image
- decode it with the current analyzer plus layout plus interpreter workflow
- classify the new highest completed stage before touching more boot code

Out of scope:

- unrelated EL-path, framebuffer, DTB, or USB work
- redesigning the whole Pi 4 boot model before the new signature-check image
  is tested

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

- the refreshed SD image is rebuilt, exported, and FAT-verified
- the next board video can now distinguish:
  - stage `31`: missing or wrong fixed load target contents
  - stage `4`: valid target contents before branch
  - stage `5`: entry veneer reached after branch

## Validation Plan

- board retry on the refreshed SD image
- decode with the current LED-analysis toolchain
- choose the next smallest boot fix from the resulting highest completed stage

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-led-analysis-toolchain.md`

## Notes

- the implementation part of the signature-check step is complete:
  - stage `4` now means armstub target signature verified at `0x40080000`
  - stage `31` now means signature mismatch and pre-branch halt
  - stage `5` now means the fixed-address `plo` entry veneer was actually
    entered after the branch
- current exported SD-image SHA-256:
  - `8ef476644f8fce5b5937096125421a218b8a67b0513b0fa4c0ab7e6592585e3e`
- initial SD-read LED chatter remains firmware preamble noise and should be
  ignored unless it participates in a later valid contiguous Phoenix decode
