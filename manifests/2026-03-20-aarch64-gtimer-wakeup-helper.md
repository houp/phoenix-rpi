# Manifest: AArch64 `gtimer` Wakeup Helper

- Date: `2026-03-20`
- Step: `STEP-0038`
- Result: `completed`

## Scope

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add a backend-state helper that arms the selected timer for a bounded relative wakeup
- preserve the existing ZynqMP timer backend and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `83666680` (`aarch64: add gtimer backend wakeup helper`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The backend-state layer can now arm the selected architectural timer for bounded positive waits using centralized conversion and register-wrapper helpers.
- Positive waits are now clamped away from zero programmed ticks inside the backend helper instead of relying on future call sites to do that conversion carefully.
- The next backend slice can focus on IRQ ownership and registration, which is now the main missing piece before public AArch64 timer-HAL takeover.

## Selected Next Step

- define the first backend IRQ-ownership helper step
