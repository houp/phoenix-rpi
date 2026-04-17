# Current Step

## Metadata

- Step ID: `STEP-0501`
- Title: `Retry Pi 4 on the syspage-tail-copy fix image`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- retry the Pi 4 with the refreshed image that fixes the likely syspage tail
  overread inside `_core_0_virtual`
- verify whether the active boundary moves past:
  - `_core_0_virtual`
  - syspage copy
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
  (SHA-256: `77164588645c65f09773165afd19eef3b7709c00fd1fc804b5dd0571003baf29`)

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
- the next real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-213826.log`
  narrowed the live boundary again to the syspage copy block itself:
  the raw tail reached `A2`, `KLM`, `X1`, `X2`, `X3`, and `NO`
- the current image therefore fixes the most plausible concrete defect there:
  the old syspage copy loop could overread the tail when `syspage->size` was
  not 8-byte aligned
