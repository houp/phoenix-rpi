# Manifest: Scope First GIC Timer-State Visibility Step

- Date: `2026-03-20`
- Step: `STEP-0155`
- Status: `completed`

## Goal

- define the smallest next experiment that can show the selected timer IRQ's GIC-side state after handler registration

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- add a tightly filtered one-time trace for the selected timer IRQ after handler registration
- expose at least:
  - the timer IRQ interrupt-group bit
  - the timer IRQ enable bit
- leave timer-source policy, timer programming, and scheduler behavior unchanged

## Why This Step

- `STEP-0150` proved that explicit non-SGI interrupt configuration does not restore timer dispatch
- `STEP-0152` proved that timer-write barriers do not restore timer dispatch
- `STEP-0154` proved that the selected timer is actually armed with `ctl 0x1` and a non-zero `tval`
- the next highest-signal bounded clue is therefore GIC state for that IRQ, not another timer-programming change

## Explicitly Deferred

- interrupt-group policy changes
- timer-source changes
- scheduler changes
- broader interrupt-controller refactoring

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/interrupts_gicv2.c`
- the next generic QEMU run exposes the timer IRQ interrupt-group and enable readback
- the result allows the following step to choose between a GIC group-policy fix and a later delivery-path diagnostic

## Selected Next Step

- implement the GIC timer-state visibility step and rerun both QEMU lanes
