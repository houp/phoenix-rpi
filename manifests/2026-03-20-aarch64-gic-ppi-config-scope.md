# Manifest: Scope First GIC PPI-Configuration Experiment

- Date: `2026-03-20`
- Step: `STEP-0149`
- Status: `completed`

## Goal

- define the smallest next experiment that can test whether leaving PPIs at reset configuration is the reason timer IRQs 27 and 30 never dispatch

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- explicitly apply `interrupts_setConf()` to non-SGI interrupts during handler registration
- keep the existing no-retargeting rule for PPIs

## Why This Step

- `STEP-0146` proved that the timer IRQ handler is registered but never dispatched
- `STEP-0148` proved that neither timer PPI 30 nor timer PPI 27 dispatches
- the generic AArch64 GIC init path currently configures only SPIs in `_hal_interruptsInit()`
- PPIs are therefore the narrowest remaining code clue that is still local to the current GIC implementation

## Explicitly Deferred

- broad GIC refactoring
- timer-source policy changes
- secure-physical timer support
- real-hardware-only conclusions

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/interrupts_gicv2.c`
- the experiment changes only interrupt configuration for non-SGI IRQs during registration
- the result will show whether explicit PPI configuration is enough to trigger timer dispatch on the fast generic lane

## Selected Next Step

- implement the PPI-configuration experiment and rerun both QEMU lanes
