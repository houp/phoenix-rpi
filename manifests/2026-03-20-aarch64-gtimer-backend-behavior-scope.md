# Manifest: First AArch64 `gtimer` Backend Behavior Helper Scope

- Date: `2026-03-20`
- Step: `STEP-0031`
- Result: `completed`

## Scope

- inspect the new backend-state layer after `STEP-0030`
- choose the smallest behavior helper to add next
- select the exact touched files for that behavior-helper step

## Result

- selected first behavior-helper slice:
  state-based raw-count and microsecond conversion helpers for the future generic timer backend
- selected responsibilities:
  - read the current architectural timer count for the state-selected source
  - convert raw timer counts to microseconds using the state-selected frequency
  - expose a convenience helper that returns current microseconds directly from backend state
- selected exact file set:
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.h`
  - `phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- selected validation:
  compile the updated backend layer in the existing `aarch64a53-zynqmp-qemu` build lane in `phoenix-dev`

## Why This Was Selected

- Future generic `hal_timerGetUs()` logic will need current time before it needs interrupt registration or wakeup programming.
- Count reading and conversion are pure backend-state behavior and do not require taking over the active timer IRQ path.
- This keeps the next step narrow and directly useful for the first public backend wrapper later.

## Selected Next Step

- implement state-based raw-count and microsecond conversion helpers in `hal/aarch64/gtimer_backend.[ch]`
