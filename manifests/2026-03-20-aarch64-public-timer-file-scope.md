# Manifest: First Common Public AArch64 Timer File Scope

- Date: `2026-03-20`
- Step: `STEP-0045`
- Result: `completed`

## Scope

- inspect the new AArch64 timer-implementation override hook after `STEP-0044`
- choose the smallest file-level step for the first common public AArch64 `hal_timer*` implementation
- select the exact validation lane for that file-level step

## Result

- selected first common public timer-file slice:
  add a new `hal/aarch64/gtimer_timer.c` file that provides the public timer entry points on top of the existing `gtimer_backend` helper layer
- selected responsibilities:
  - own a local common-AArch64 timer state instance in the new file
  - provide the required public timer entry points from one common implementation file
  - validate compilation through the new `AARCH64_TIMER_IMPL_OVERRIDE` hook rather than changing the default ZynqMP timer selection
- selected exact file set:
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
  - coordination-repo manifests and tracking files
- selected validation:
  rebuild the existing `aarch64a53-zynqmp-qemu` lane in `phoenix-dev` with
  `AARCH64_TIMER_IMPL_OVERRIDE='$(addprefix $(PREFIX_O)hal/aarch64/, gtimer_timer.o)'`
  so the new common file is compiled as the selected timer implementation without changing the default target configuration

## Why This Was Selected

- The helper layer and build hooks are now sufficient for a first real common public timer implementation file.
- Compiling that file through the explicit override hook keeps the validation lane narrow and avoids premature target-definition work.
- This keeps the next step focused on one new file instead of mixing runtime selection, target metadata, and common timer code in one patch.

## Selected Next Step

- implement the first common public AArch64 timer implementation file in `hal/aarch64/gtimer_timer.c`
