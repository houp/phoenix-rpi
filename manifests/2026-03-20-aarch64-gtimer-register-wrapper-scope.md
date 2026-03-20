# Manifest: First AArch64 `gtimer` Register Wrapper Scope

- Date: `2026-03-20`
- Step: `STEP-0035`
- Result: `completed`

## Scope

- inspect the backend-state helper layer after `STEP-0034`
- choose the smallest state-keyed register wrapper slice needed before timer-arming policy
- select the exact touched files for that wrapper step

## Result

- selected first timer-register wrapper slice:
  backend-state wrappers for reading the control register, writing the control register, and programming relative timer ticks through the state-selected timer source
- selected responsibilities:
  - expose state-keyed control access without open-coded source lookups
  - expose state-keyed relative timer programming without introducing wakeup policy
  - keep all changes inside `hal/aarch64/gtimer_backend.[ch]`
- selected exact file set:
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.h`
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- selected validation:
  compile the updated backend layer in the existing `aarch64a53-zynqmp-qemu` build lane in `phoenix-dev`

## Why This Was Selected

- Future generic `hal_timerSetWakeup()` logic will need backend-owned control and timer-register access before it can define arming policy.
- The backend already owns the selected timer source, so direct state-keyed wrappers avoid repeating `state->source` plumbing in later steps.
- This keeps the next implementation step mechanical and reviewable while still moving the backend closer to a public timer wrapper.

## Selected Next Step

- implement backend-state wrappers for control access and relative timer programming in `hal/aarch64/gtimer_backend.[ch]`
