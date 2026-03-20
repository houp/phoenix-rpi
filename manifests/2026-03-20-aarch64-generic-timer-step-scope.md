# Manifest: First Common AArch64 Generic Timer Step Scope

- Date: `2026-03-20`
- Step: `STEP-0019`
- Result: `completed`

## Scope

- inspect the current AArch64 timer preparation work
- choose the first common generic timer backend shape
- choose the first timer source policy
- select the next smallest implementation step

## Result

- selected eventual backend shape:
  a directly selectable common AArch64 generic timer implementation
- selected first timer-source policy for common EL1 bring-up:
  - prefer the non-secure physical timer
  - fall back to the virtual timer if the non-secure physical timer is not described
  - do not select the secure or hypervisor timer in the current common EL1 path
- selected next implementation step:
  add AArch64 generic timer source selection helpers to the DTB API so the future common timer backend can consume a single explicit source decision instead of open-coding policy

## Why This Was Selected

- The timer HAL API, DTB timer metadata, and AArch64 timer sysreg helpers now exist, but the source-selection policy is still implicit.
- Encoding that policy first keeps the actual backend step smaller and easier to review.
- It also keeps the first backend implementation focused on programming the chosen timer, not on policy and register selection at the same time.

## Source Basis

- Linux `drivers/clocksource/arm_arch_timer.c` selection logic indicates that arm64 kernels do not use the secure timer and typically prefer the non-secure physical timer path, with virtual-timer use as a fallback case.
- The local QEMU `virt` timer DT node exposes the standard interrupt tuple order already parsed by Phoenix:
  secure physical, non-secure physical, virtual, hypervisor.

## Selected Next Step

- update `phoenix-rtos-kernel/hal/aarch64/dtb.h`
- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- add a small AArch64 generic-timer source selection API that returns:
  - the selected source kind
  - the selected interrupt number
- keep build validation on the current `aarch64a53-zynqmp-qemu` lane in `phoenix-dev`
