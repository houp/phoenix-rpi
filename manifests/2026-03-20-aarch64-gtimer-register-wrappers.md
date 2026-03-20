# Manifest: AArch64 `gtimer` Register Wrappers

- Date: `2026-03-20`
- Step: `STEP-0036`
- Result: `completed`

## Scope

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add backend-state wrappers for control register access and relative timer programming
- preserve the existing ZynqMP timer backend and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `b75665bb` (`aarch64: add gtimer backend register wrappers`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The backend-state layer now owns direct access to architectural timer control and relative timer registers through state-keyed wrappers.
- Future backend steps can now focus on timer-arming policy and IRQ ownership instead of repeating source dispatch or raw register access.
- The next backend slice can introduce a narrow arming helper without having to reopen the source-selection plumbing.

## Selected Next Step

- define the first backend timer-arming policy step on top of the current backend wrappers
