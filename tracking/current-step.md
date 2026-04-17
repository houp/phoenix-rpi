# Current Step

## Metadata

- Step ID: `STEP-0509`
- Title: `Retry Pi 4 without the temporary post-MMU PL011 debug path`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- retry the Pi 4 on the image that removes the temporary high-half PL011 debug
  mapping and post-MMU UART breadcrumbs
- verify whether the real hardware path can now continue into the normal kernel
  console path without that probe seam

## Scope

In scope:

- one real-device retry on the refreshed image
- UART capture with the canonical helper
- no LED diagnostics
- no new source probes before the retry result

Out of scope:

- more kernel probe churn
- broader userspace tracing
- DTB refactors before this hardware retry

## Acceptance Criteria

- the refreshed image is tried on real hardware
- the retry captures at least one raw UART log
- the retry shows whether execution moves beyond:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
- the retry proves whether the board finally reaches the normal kernel console
  path beyond the old temporary `X1 / X2 / X3` seam

## Validation Plan

- flash:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- capture UART with:
  - `scripts/capture-rpi4b-uart.sh --profile firmware ...`
- inspect the resulting log for:
  - `AS0`
  - `TR0..TR3`
  - `hal: jump exit el1`
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
  - `console: pl011 init done`
  - `main: hal init done`
  - the kernel banner

## Rollback / Baseline

- current exported image to test:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `358f4325dec6009e0b9441c052dad370b2fedeb81a6f93eb43db1eadd06f750a`)

## Notes

- the stale-image theory has been disproved for the current artifact chain
- the exact rollback image previously stopped on real hardware at:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
- a bounded Pi 4 QEMU gdbstub session proved that same image still reaches:
  - `_core_0_virtual`
  - `_set_up_vbar_and_stacks`
  - `main()`
  under emulation
- the current image keeps the earlier `TTBR1`-from-start structure and the
  pre-MMU page-table invalidation fix
- the only removed piece is the temporary high-half PL011 debug path, because it
  had become the strongest candidate for a self-inflicted hardware-only fault
