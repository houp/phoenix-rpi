# Current Step

## Metadata

- Step ID: `STEP-0502`
- Title: `Retry Pi 4 on the pre-MMU syspage-copy image`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- retry the Pi 4 with the refreshed image that moves the syspage copy before
  the MMU-backed `_core_0_virtual` handoff and enlarges the syspage backing
  buffer
- verify whether the active boundary moves past:
  - the old `... NO` seam
  - pre-MMU syspage relocation
  - `_set_up_vbar_and_stacks`
  - earliest `main()`

## Scope

In scope:

- one real-device retry on the refreshed image
- UART capture with the canonical helper
- no LED dependence
- no new broad tracing before the retry result

Out of scope:

- restoring LED diagnostics
- broader userspace tracing
- unrelated DTB/runtime refactors before the next retry

## Acceptance Criteria

- the refreshed image is tried on real hardware
- the retry captures at least one raw UART log
- the retry shows whether the raw tail still stops at `NO` or moves forward to
  at least `P`
- the next engineering change can target one precise sub-band of the kernel
  MMU-to-`main()` path

## Validation Plan

- flash:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- capture UART with:
  - `scripts/capture-rpi4b-uart.sh --profile firmware ...`
- inspect the resulting log for:
  - `AS0`
  - `TR0..TR3`
  - `hal: jump exit el1`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
  - `N`
  - `O`
  - `P`
  - `Q`
  - `R`
  - `S`

## Rollback / Baseline

- current exported image to test:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `5d6f4bba3786543db10132cf2febf1ebdd37d819e780d795e611bdc141bb422e`)

## Notes

- the latest real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-215745.log`
  disproved the previous tail-copy hypothesis:
  - the raw tail still reaches:
    - `A2`
    - `KLM`
    - `X1`
    - `X2`
    - `X3`
    - `NO`
  - and still does not reach:
    - `P`
    - `Q`
    - `R`
    - `S`
- that means:
  - the board survives MMU enable and reaches `_core_0_virtual`
  - the byte-tail copy fix did not move the live hardware boundary
  - the more fragile design is the whole post-MMU syspage-copy step itself
- the refreshed image therefore applies a semantic simplification:
  - copy the syspage to its kernel backing before the MMU-backed virtual jump,
    matching the simpler pattern used by the older ARM and RISC-V ports
  - enlarge `_hal_syspageCopied` from one page to `16 * SIZE_PAGE` so the
    copied syspage is not constrained to a 4 KB buffer
