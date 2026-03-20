# Manifest: First Generic AArch64 Timer Backend Skeleton Step Scope

- Date: `2026-03-20`
- Step: `STEP-0029`
- Result: `completed`

## Scope

- inspect the current `gtimer` helper layer, timer hook, and DTB source-selection API after `STEP-0028`
- choose the smallest backend-skeleton responsibility to introduce next
- select the exact touched files for that backend-skeleton step

## Result

- selected first backend-skeleton responsibility:
  define and initialize generic AArch64 timer backend state without taking ownership of the public `hal_timer*` API yet
- selected backend-state contents:
  - selected timer source
  - selected interrupt number
  - architectural timer frequency
- selected exact file set:
  - `phoenix-rtos-kernel/hal/aarch64/Makefile`
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.h`
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- selected validation:
  compile the backend skeleton in the existing `aarch64a53-zynqmp-qemu` build lane in `phoenix-dev` while keeping the ZynqMP timer backend active

## Why This Was Selected

- The next real backend work needs one place to hold the chosen timer source, IRQ, and frequency.
- DTB source selection and low-level sysreg access are already available, so state initialization is now the smallest missing backend-specific layer.
- Keeping the public `hal_timer*` entry points out of this step avoids jumping too early into backend activation or runtime replacement.

## Selected Next Step

- implement the generic AArch64 timer backend state skeleton in `hal/aarch64/gtimer_backend.[ch]` and compile it in the current common AArch64 build
