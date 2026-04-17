# Current Step

## Metadata

- Step ID: `STEP-0511`
- Title: `Verify Pi 4 kernel boot past MMU-on and GIC init`
- Status: `in_progress`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- verify that the MMU identity table zeroing fix resolves the hang at `3C`
- verify that the DTB GIC parsing fix allows interrupt initialization to succeed
- verify that the TLB invalidation move ensures DTB mapping visibility
- reach the kernel banner and ideally the `psh` shell on real hardware

## Scope

In scope:

- validation of the `phoenix-rtos-kernel` fixes in `_init.S`, `pmap.c`, and `dtb.c`
- one rebuilt and re-exported Pi 4 image
- one real-device UART retry on that image

Out of scope:

- new peripheral driver development
- further DTB refactoring

## Acceptance Criteria

- the refreshed image is tried on real hardware
- the boot process proceeds beyond marker `3C`
- the kernel banner is printed to the UART console
- the `psh` shell prompt is reached

## Validation Plan

- rebuild and flash:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
- capture UART with:
  - `scripts/capture-rpi4b-uart.sh --profile firmware ...`
- inspect the resulting log for:
  - `3C`
  - `Phoenix-RTOS v. 3.3.1` (kernel banner)
  - `(psh)%`

## Rollback / Baseline

- current exported image to test:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `4e873f294f07e6d636390816aac318b51f3ceb55ed85ab4ea9ac594e0fc06204`)
- previous image before this strategy change:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `bc08128b86c3d7b22cbd1160b81281d0ef5849c34c88f962b3cadfad29aa559d`)

## Notes

- the stale-image theory has been disproved for the current artifact chain
- the latest real-board UART log
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260417-234142.log`
  still stopped at:
  - `A2`
  - `KLM`
  - `X1`
  - `X2`
  - `3C`
- earlier real-board logs on the same day genuinely reached `...X3NO`, so the
  project is not stuck on an imaginary or stale-image boundary; it lost
  observability after the post-MMU UART path was removed
- the next move is therefore to catch the first real exception at the seam,
  not to add more late-stage progress markers
