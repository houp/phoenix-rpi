# Manifest: AArch64 `gtimer` Backend Wait-To-Ticks Helper

- Date: `2026-03-20`
- Step: `STEP-0034`
- Result: `completed`

## Scope

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add a backend-state helper that converts relative microseconds to architectural timer ticks
- preserve the existing ZynqMP timer backend and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `aecd438c` (`aarch64: add gtimer backend wait-to-ticks helper`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The backend-state layer now centralizes both directions of time conversion: counts to microseconds and microseconds to relative timer ticks.
- Forward conversion can now be reused by future timer-arming helpers instead of leaving raw frequency math at call sites.
- The next backend slice can focus on state-keyed timer-register wrappers without reopening conversion policy.

## Selected Next Step

- define the first backend timer-register wrapper step on top of the current backend state and conversion helpers
