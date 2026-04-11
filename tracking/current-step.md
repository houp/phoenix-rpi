# Current Step

## Metadata

- Step ID: `STEP-0466`
- Title: Await the first real Pi 4 retry on the relocatable kernel8 trampoline image
- Status: `in_progress`
- Date: `2026-04-11`
- Milestone / phase: `Phase 1`

## Objective

- validate the new relocatable `kernel8.img` trampoline on real Pi 4 hardware
- confirm through UART whether the live board now reaches:
  - `TR0`
  - `TR1`
  - `TR2`
  - `TR3`
- determine whether the previous relocation mismatch was the remaining blocker

## Scope

In scope:

- running the next real Pi 4 board retry on the refreshed SD image
- capturing UART plus LED video in parallel
- classifying the highest reached phase from the UART log first
- using LED only as a secondary cross-check if the UART path is still partial

Out of scope:

- further image-format changes before the new real-device evidence arrives
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

- a real Pi 4 UART log is captured on the refreshed image
- the log is classified with the canonical helper without ignoring warnings
- the retry proves one of:
  - the board reaches `TR0..TR3` and then later Phoenix code
  - the board reaches only a proper prefix of `TR0..TR3`
  - the board still never reaches the trampoline
- the next code step is chosen from that UART evidence, not from LED-only guesswork

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
  `/Users/witoldbolt/phoenix-rpi/manifests/2026-04-11-pi4-kernel8-reloc-trampoline.md`

## Notes

- the last decisive UART log proved:
  - `Loaded 'kernel8.img' to 0x40080000`
  - `Kernel relocated to 0x80000`
- the new image is the direct response to that evidence:
  - `kernel8.img` is now a relocatable trampoline with `TR0..TR3`
  - the embedded high-linked `plo` payload is copied to `0x40080000`
  - the copied region now gets explicit cache maintenance before branch
- current exported SD-image SHA-256:
  `610dbbfd0192760f061395f7e85573261b85b18857bea426e6adab4930468698`
