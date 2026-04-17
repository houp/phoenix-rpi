# Current Step

## Metadata

- Step ID: `STEP-0505`
- Title: `Retry Pi 4 on the TTBR1 cache-maintenance fix image`
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- retry the Pi 4 with the refreshed image that removes the invalid post-`ttbr1`
  raw UART probes and adds explicit TTBR1 page-table cache/TLB maintenance
- verify whether the active boundary moves past:
  - the old `... X3` silent gap
  - the old `... NO` seam
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
- the retry shows whether execution moves beyond the old `... X3` silent gap
  after the TTBR1 maintenance fix
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
  (SHA-256: `725744d23cbd7bf08080c52ec02230269f680cd554ffc8c3d23a27b31f30ec2c`)

## Notes

- the latest real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-222617.log`
  still ends at:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `X3`
- this, combined with the older status note already recorded in
  `docs/status.md`, makes the next conclusion explicit:
  - raw UART probes after the `ttbr1_el1` switch are not a reliable method on
    this path and have been perturbing bring-up
  - the remaining stronger hypothesis is real-hardware page-table visibility,
    not lack of enough breadcrumbs
- the refreshed image therefore:
  - removes all post-`ttbr1` raw UART probes from
    `phoenix-rtos-kernel/hal/aarch64/_init.S`
  - removes the matching stale early-UART hook from
    `phoenix-rtos-kernel/main.c`
  - removes the now-unused `PL011_TTY_EARLY_VADDR` board-config define from
    `phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
  - adds explicit cache/TLB maintenance for TTBR1 page tables before enabling
    the `ttbr1` path:
    - clean D-cache for the kernel TTBR1 tables
    - `tlbi vmalle1is`
- the previous regression rollback remains true too:
  - `_hal_syspageCopied` is back to `SIZE_PAGE`
- the refreshed image no longer attempts to classify:
  - `N`
  - `O`
  - `U`
  - `V`
  - `W`
  - `Z`
  - `Y`
  - `P`
- instead, the next retry should simply answer whether the TTBR1 fix restores
  later boot continuity
    - `W` after `syspage->size` load
    - `Z` before the first copy iteration
    - `Y` after the first 8-byte copy iteration
    - `P` after full copy completion
