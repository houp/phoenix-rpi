# Manifest: Scope First Architectural-Timer Register-Readback Experiment

- Date: `2026-03-20`
- Step: `STEP-0153`
- Status: `completed`

## Goal

- define the smallest next experiment that can show whether the selected architectural timer is actually left armed after wakeup programming

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- add minimal timer readback helpers for the selected architectural timer
- emit a one-time post-arm trace that shows the resulting control state and timer value
- leave timer-source selection, GIC configuration, and scheduler behavior unchanged

## Why This Step

- `STEP-0148` proved that switching from the non-secure physical timer to the virtual timer does not restore dispatch
- `STEP-0150` proved that explicit non-SGI interrupt configuration does not restore dispatch
- `STEP-0152` proved that explicit barriers after timer sysreg writes also do not restore dispatch
- the next highest-signal bounded clue is whether wakeup programming actually leaves a non-zero countdown and an enabled control state in the selected timer

## Explicitly Deferred

- interrupt-group policy changes
- broader timer redesign
- broader exception or scheduler debugging
- real-hardware-only conclusions

## Acceptance Criteria

- the next implementation patch stays in the common AArch64 timer helper path
- the next generic QEMU run exposes the programmed timer control state and timer value after arming
- the next result allows the following step to distinguish timer-programming failure from later interrupt-delivery failure

## Selected Next Step

- implement the architectural-timer register-readback experiment and rerun both QEMU lanes
