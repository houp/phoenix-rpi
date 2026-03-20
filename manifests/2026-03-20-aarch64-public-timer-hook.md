# Manifest: AArch64 Public Timer Hook

- Date: `2026-03-20`
- Step: `STEP-0042`
- Result: `completed`

## Scope

- update `hal/aarch64/Makefile`
- update `hal/aarch64/zynqmp/Makefile`
- split the AArch64 timer build glue so the public timer implementation object is selected explicitly
- preserve the existing ZynqMP timer implementation and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `353627ea` (`aarch64: split public timer implementation hook`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The AArch64 build now has an explicit public timer-implementation hook, while the current ZynqMP timer implementation remains selected and behavior-preserving.
- The next common timer step no longer needs to reopen Makefile glue before adding the first public `hal_timer*` wrapper file.
- The project is now at the first real common public timer-HAL boundary rather than a backend-helper or build-glue boundary.

## Selected Next Step

- define the first public common AArch64 timer-HAL wrapper step
