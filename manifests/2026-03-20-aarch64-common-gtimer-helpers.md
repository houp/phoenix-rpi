# Manifest: Common AArch64 `gtimer` Helpers

- Date: `2026-03-20`
- Step: `STEP-0028`
- Result: `completed`

## Scope

- add `hal/aarch64/gtimer.h`
- add `hal/aarch64/gtimer.c`
- compile the helper in the current common AArch64 build
- preserve the existing ZynqMP runtime path and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `09963e1e` (`aarch64: add common gtimer helpers`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The common AArch64 build now compiles a `gtimer` helper layer that hides the physical-versus-virtual timer sysreg split behind a source-keyed API.
- This keeps the next architectural timer backend slice focused on backend state and policy instead of low-level sysreg branching.
- The current ZynqMP runtime path remains unchanged, because the helper layer is compiled but not yet selected as the active timer backend.

## Selected Next Step

- define the first generic AArch64 timer backend skeleton step on top of the new helper layer
