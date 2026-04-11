# Current Step

## Metadata

- Step ID: `STEP-0469`
- Title: Await the next Pi 4 retry on the armstub fallback-to-`0x80000` image
- Status: `in_progress`
- Date: `2026-04-11`
- Milestone / phase: `Phase 1`

## Objective

- validate whether the new armstub fallback from empty `kernel_entry32` to
  `0x80000` lets the board reach trampoline UART `TR0`
- capture both UART and LED again on the refreshed image
- decide whether the next blocker is now later than the armstub contract

## Scope

In scope:

- running the next real Pi 4 retry on the refreshed fallback image
- capturing UART and LED in parallel again
- classifying whether stage `30` and then trampoline UART `TR0..TR3` appear

Out of scope:

- unrelated UART-host tool redesign
- network-boot lab setup
- unrelated Pi 4 driver work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- `/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh`
- `/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py`
- `/Users/witoldbolt/phoenix-rpi/docs/pi4-first-hardware-trial.md`
- `/Users/witoldbolt/phoenix-rpi/docs/status.md`

## Acceptance Criteria

- a real Pi 4 retry is captured on image
  `4d9daf70168d6990e7525d0c0accda4a8a1ffed0a5fe62432aab4dcff8e70217`
- the new evidence proves one of:
  - `TR0` now appears
  - the new fallback stage `30` appears and then `TR0..TR3` appear
  - the board still fails before `TR0`, which would keep suspicion on the
    remaining armstub DTB / branch contract

## Validation Plan

- flash:
  [verify-rpi4b-sdimg.sh](/Users/witoldbolt/phoenix-rpi/scripts/verify-rpi4b-sdimg.sh)
- capture:
  [capture-rpi4b-uart.sh](/Users/witoldbolt/phoenix-rpi/scripts/capture-rpi4b-uart.sh)
- summarize:
  [summarize-rpi4b-uart-log.py](/Users/witoldbolt/phoenix-rpi/scripts/summarize-rpi4b-uart-log.py)
- optional secondary signal:
  LED video plus the existing analysis toolchain

## Rollback / Baseline

- latest implementation manifest:
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-11-pi4-armstub-kernel-entry-fallback.md`

## Notes

- the last real retry showed:
  - firmware now loads `loader.disk` to `0x08000000`
  - firmware now loads `kernel8.img` to `0x00200000`
  - firmware still relocates `kernel8.img` to `0x80000`
  - LED decode reaches the special `31` path, proving the armstub still saw an
    empty `kernel_entry32`
- the refreshed image now keeps the firmware-slot path when present, but falls
  back to the observed real-firmware relocation target:
  - `0x00080000`
- current exported SD-image SHA-256:
  `4d9daf70168d6990e7525d0c0accda4a8a1ffed0a5fe62432aab4dcff8e70217`
