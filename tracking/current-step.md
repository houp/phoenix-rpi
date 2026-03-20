# Current Step

## Metadata

- Step ID: `STEP-0026`
- Title: Implement CPU0-directed timer wakeup notification
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- implement the first narrow timer-path change that lets non-CPU0 contexts request CPU0 wakeup-timer reprogramming without changing the timer backend itself yet

## Scope

In scope:

- reserve `TIMER_WAKEUP_IRQ` in the AArch64 interrupt definitions
- add a guarded remote wakeup-notification path in `proc/threads.c`
- coalesce remote wakeup requests and handle them on CPU 0 under `threads_common.spinlock`
- validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- changing the timer backend implementation itself
- widening into generic IPI policy or TLB integration
- implementing the common generic timer runtime backend itself
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/arch/interrupts.h`
- `proc/threads.c`
- tracking files and manifest updates for this step

## Acceptance Criteria

- AArch64 reserves `TIMER_WAKEUP_IRQ` explicitly for the wakeup notification path
- `proc/threads.c` coalesces remote wakeup requests and notifies CPU 0 through the targeted SGI helper
- CPU 0 handles that notification by recomputing the wakeup deadline under the scheduler spinlock
- the existing `aarch64a53-zynqmp-qemu` build still succeeds in `phoenix-dev`

## Validation Plan

- Build:
  refresh the copied buildroot and rebuild `TARGET=aarch64a53-zynqmp-qemu` with `./phoenix-rtos-build/build.sh clean host core project`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-timer-wakeup-notification-step-scope.md`

## Notes

- Risks:
  the step must remain scheduler-local and avoid widening into a broader architectural timer backend change
- Dependencies:
  completed scoping step from `STEP-0025`
- User-visible control point before next step:
  after this step lands, the next work should either tighten the wakeup contract or start the smallest architectural timer backend slice, but not both at once
