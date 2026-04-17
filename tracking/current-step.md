# Current Step

## Metadata

- Step ID: `STEP-0500`
- Title: `Retry Pi 4 on the pre-MMU UART split image`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- retry the Pi 4 with the refreshed image that adds both pre-MMU and post-MMU
  kernel UART breadcrumbs
- classify the remaining real-hardware boundary between:
  - early MMU setup before `ttbr0_el1`
  - `ttbr0_el1` programming
  - actual MMU enable at `msr sctlr_el1, x0`
  - first safe post-MMU UART point
  - later `_core_0_virtual` / `main()` path if the boundary moves

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
- the retry shows the highest completed kernel breadcrumb among:
  - `KLM`
  - `KLM + X1`
  - `KLM + X1 + X2`
  - `KLM + X1 + X2 + X3`
  - `X3` plus no `N`
  - or a later `N..S` continuation
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
  (SHA-256: `fe9d163ab5d23aa88bbaea35c5df790f48b76dea58d1cd843b8cd56990c74273`)

## Notes

- the latest real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-211048.log`
  already proves:
  - firmware handoff is working
  - custom armstub UART recovery is working
  - reloc trampoline UART recovery is working
  - `plo` reaches and exits via:
    - `hal: jump entry`
    - `hal: jump irq off`
    - `hal: jump exit el1`
  - kernel `_start` reaches:
    - `K`
    - `L`
    - `M`
- the follow-up real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-213033.log`
  proved that the first post-MMU split was still too late, because the raw
  tail remained exactly `A2` then `KLM`
- the active blocker is now between `M` and the first safe post-MMU UART
  breadcrumb
- the current image therefore adds three earlier physical-UART checkpoints
  `X1/X2/X3` before the MMU enable, while keeping the later fixed-virtual-UART
  `N..S` checkpoints in place
