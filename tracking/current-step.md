# Current Step

## Metadata

- Step ID: `STEP-0497`
- Title: `Retry Pi 4 on the no-LED UART baseline`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- verify that the remaining Pi 4 GPIO42 LED debug path was the direct cause of
  the late `plo` stall
- keep UART as the primary lane
- push the real board past:
  - `hal: jump entry`
  - `hal: jump irq off`
  - `hal: jump exit el1`
  - earliest kernel output

## Scope

In scope:

- one real-device retry on the refreshed image
- firmware-profile UART capture
- fallback postswitch capture only if needed
- HDMI observation
- no GPIO42/ACT-LED dependency

Out of scope:

- restoring LED diagnostics
- new broad probe maps
- unrelated DTB/runtime refactors before the next retry

## Acceptance Criteria

- the refreshed no-LED image is tried on real hardware
- the retry captures at least one raw UART log
- the retry shows whether execution now progresses past `hal: jump entry`
- the next engineering step can target either:
  - EL1 handoff / earliest kernel entry, or
  - a later kernel/userspace seam

## Validation Plan

- flash:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- capture UART with:
  - `scripts/capture-rpi4b-uart.sh --profile firmware ...`
- check the resulting log for:
  - `AS0`
  - `TR0..TR3`
  - `hal: jump irq off`
  - `hal: jump exit el1`
  - earliest kernel output
- use `--profile postswitch` only if a `firmware` run still never resumes
  after the firmware baud-switch line

## Rollback / Baseline

- current image to test:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `69fe152d093a4cd5d36d250034d7ce726b7e70f4520a4b8cec50bedc4faf74a2`)
- previous UART-continuity image with remaining LED delays:
  `8d4770cdf96a6af16fb1a1c85c75cdd267aff839caf8998f523dd2dac4a9ee15`

## Notes

- the immediately previous real-board UART log already proved:
  - firmware handoff is working
  - custom armstub UART recovery is working
  - reloc trampoline UART recovery is working
  - `plo` is running and reaches:
    - `go: enter`
    - `go: devs done`
    - `go: hal done`
    - `hal: jump entry`
- the active blocker on that image was the remaining LED-delay code inside
  `video_markKernelJump()` and the kernel-side Pi 4 GPIO42 diagnostics
- this step intentionally removes those diagnostics entirely rather than merely
  shortening them
