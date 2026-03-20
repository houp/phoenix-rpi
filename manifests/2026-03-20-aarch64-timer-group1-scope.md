# Manifest: Scope First Timer-IRQ Group 1 Experiment

- Date: `2026-03-20`
- Step: `STEP-0157`
- Status: `completed`

## Goal

- define the smallest next experiment that can test whether moving only the selected timer IRQ to Group 1 restores PPI enablement and dispatch

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- add a minimal interrupt-group helper
- set only the selected timer IRQ to Group 1 before enabling it
- preserve the existing timer IRQ state trace so the next run shows both group and enable readback
- leave all non-timer IRQ group policy unchanged

## Why This Step

- `STEP-0154` proved the selected timer is genuinely armed
- `STEP-0156` proved the selected timer IRQ still reads back as `grp 0 en 0`
- `sources/plo/hal/aarch64/generic/_init.S` exits EL3 to EL1 non-secure
- `sources/plo/hal/aarch64/zynqmp/_init.S` already contains an explicit comment and implementation that move interrupts to Group 1 so non-secure code can manage them
- the smallest high-signal fix is therefore a timer-only Group 1 experiment, not a broader GIC policy rewrite

## Explicitly Deferred

- changing group policy for all interrupts
- changing timer-source selection
- scheduler changes
- broader GIC refactoring

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/interrupts_gicv2.c`
- the next generic QEMU run exposes whether the timer IRQ changes from `grp 0 en 0` to an enabled Group 1 state
- the result allows the following step to choose between generalizing Group 1 policy and another narrower GIC delivery fix

## Selected Next Step

- implement the timer-IRQ Group 1 experiment and rerun both QEMU lanes
