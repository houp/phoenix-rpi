# Manifest: AArch64 `gtimer` IRQ Helpers

- Date: `2026-03-20`
- Step: `STEP-0040`
- Result: `completed`

## Scope

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add backend-state helpers for querying the selected timer IRQ and registering a handler against it
- preserve the existing ZynqMP timer backend and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `ed276528` (`aarch64: add gtimer backend irq helpers`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The backend-state layer now exposes the selected timer IRQ and can register handlers against it without open-coded backend-state plumbing.
- The common AArch64 timer backend now has backend-local support for time reads, wakeup arming, and IRQ ownership.
- The next step can finally move to the first public timer-HAL wrapper boundary; helper-layer work is no longer the main blocker.

## Selected Next Step

- define the first public common AArch64 timer-HAL wrapper step
