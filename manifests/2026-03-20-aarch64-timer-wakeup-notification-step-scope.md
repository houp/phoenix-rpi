# Manifest: First CPU0-Directed Timer Wakeup Notification Step Scope

- Date: `2026-03-20`
- Step: `STEP-0025`
- Result: `completed`

## Scope

- inspect the current AArch64 SGI and timer preparation state after `STEP-0024`
- determine how a timer-update notification should reserve or reuse an interrupt number
- determine which component should own the first CPU0-directed notification handler
- select the smallest exact file set for the first runtime-oriented timer-notification patch

## Result

- selected interrupt-number strategy:
  reserve SGI `1` for the first AArch64 timer wakeup notification path as `TIMER_WAKEUP_IRQ`
- selected ownership:
  the first CPU0-directed notification handler should live in `proc/threads.c`, because the wakeup recomputation already belongs to the thread scheduler and already runs under `threads_common.spinlock`
- selected first runtime-oriented step:
  add a guarded wakeup-notification path in `proc/threads.c` that:
  - coalesces remote wakeup requests with a `timerWakeupPending` flag
  - sends a targeted SGI to CPU 0
  - handles that SGI on CPU 0 by recomputing the next timer deadline through the existing scheduler wakeup logic
- selected exact file set:
  - `phoenix-rtos-kernel/hal/aarch64/arch/interrupts.h`
  - `phoenix-rtos-kernel/proc/threads.c`

## Why This Was Selected

- The scheduler already owns sleeping-thread ordering and next-deadline computation, so moving the first CPU0-directed wakeup notification there keeps the timer backend itself smaller.
- The new targeted SGI helper from `STEP-0024` means the first notification path does not need broader interrupt plumbing.
- Reserving a dedicated SGI keeps the first timer-notification change explicit and reviewable instead of burying it in a more generic IPI policy.

## Validation Lane For The Next Step

- refresh the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- rebuild:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`

## Selected Next Step

- implement the first CPU0-directed timer wakeup-notification path in `proc/threads.c` and `hal/aarch64/arch/interrupts.h`
