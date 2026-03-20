# Manifest: Scope First Timer-Source Selection Experiment

- Date: `2026-03-20`
- Step: `STEP-0147`
- Status: `completed`

## Goal

- define the smallest next experiment that can test whether the current common AArch64 timer-source preference is the reason IRQ 30 never dispatches

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- change only the timer-source preference order in `dtb_chooseTimerSource()`
- prefer the virtual timer over the physical non-secure timer when both are available

## Why This Step

- `STEP-0144` proved the current common policy selects `physical-nonsecure irq 30`
- `STEP-0146` proved that IRQ 30 is registered in GICv2 but never dispatched
- the smallest common policy experiment is therefore to keep the same generic timer backend and GIC wiring, but select the alternate architectural timer source already exposed by the DTB

## Explicitly Deferred

- broad timer or GIC refactoring
- secure-physical timer support
- real-hardware-only conclusions
- non-diagnostic service-order workarounds

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/dtb.c`
- the experiment changes only timer-source preference order
- the result will show whether switching from non-secure physical to virtual timer changes GIC dispatch on the generic fast lane

## Selected Next Step

- implement the virtual-first timer-source experiment and rerun the generic and Pi 4 DTB-backed QEMU lanes
