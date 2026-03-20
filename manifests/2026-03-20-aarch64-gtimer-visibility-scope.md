# Manifest: Scope Common AArch64 Timer Source / IRQ Visibility

- Date: `2026-03-20`
- Step: `STEP-0143`
- Status: `completed`

## Goal

- define the smallest next diagnostic step that can expose which common AArch64 timer source and IRQ are selected and armed before the missing timer interrupt should arrive

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- add tightly filtered, one-time common AArch64 timer markers for:
  - selected timer source
  - selected IRQ number
  - first wakeup arming

## Why This Step

- `STEP-0142` already proved that the blocked retry path reaches `proc_threadNanoSleep()` and that `_threads_programWakeup()` calls `hal_timerSetWakeup()`
- `gtimer_timer.c` already owns:
  - `timer_common.state.source`
  - `hal_gtimerStateIrq(&timer_common.state)`
  - `hal_gtimerStateSetWakeup(&timer_common.state, waitUs)`
- that means the next visibility patch can stay inside one common AArch64 timer file rather than widening into DTB parsing, backend state initialization, or GIC code

## Explicitly Deferred

- changing timer-source policy
- changing IRQ routing or GIC configuration
- broad timer-backend refactoring
- real-hardware-only debugging

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/gtimer_timer.c`
- the marker plan exposes selected source, IRQ, and first wakeup arming
- the scope preserves the generic `virt` and Pi 4 DTB-backed QEMU lanes as the current validation targets

## Selected Next Step

- implement filtered source / IRQ / arm visibility in `hal/aarch64/gtimer_timer.c` and rerun both QEMU lanes
