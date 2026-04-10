# Current Step

## Metadata

- Step ID: `STEP-0458`
- Title: Split the armstub pre-signature-read band after stage `3`
- Status: `in_progress`
- Date: `2026-04-10`
- Milestone / phase: `Phase 1`

## Objective

- distinguish the three remaining possibilities inside the narrow armstub seam
  after stage `3`:
  - fault before even loading the fixed `0x40080000` target
  - fault on the first or second signature-memory read
  - successful reads followed by a signature mismatch path
- keep the LED-analysis toolchain as the default readout path for the next
  board retry

## Scope

In scope:

- add one or two special armstub-only stage codes between:
  - stage `3`
  - first fixed-target read
  - second fixed-target read
  - existing stage `31` mismatch halt
- keep the later `plo` stage map stable

Out of scope:

- unrelated EL-path, framebuffer, DTB, or USB work
- redesigning the whole Pi 4 boot model before the first fixed-target read
  seam is answered

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

- the next image adds narrower armstub-side checkpoints without shifting the
  later `plo` stage map again
- the next board video can distinguish whether failure occurs:
  - before fixed-target address load
  - on the first or second target-memory read
  - on the signature-mismatch halt path

## Validation Plan

- rebuild Pi 4 image
- rerun the strongest no-hardware gates
- board retry plus LED decode

## Rollback / Baseline

- Known-good manifest or commit set:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-10-pi4-led-analysis-toolchain.md`

## Notes

- current confirmed decode from `IMG_7138.mov`:
  - best contiguous Phoenix run:
    - stage `3`
  - no later valid stage `4`
  - no valid stage `31`
  - unmatched earlier `16` and later `26` bursts are treated as noise, not the
    main run
- current exported SD-image SHA-256:
  - `8ef476644f8fce5b5937096125421a218b8a67b0513b0fa4c0ab7e6592585e3e`
- initial SD-read LED chatter remains firmware preamble noise and should be
  ignored unless it participates in a later valid contiguous Phoenix decode
