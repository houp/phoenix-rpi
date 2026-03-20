# Manifest: Scope First Architectural-Timer Write-Barrier Experiment

- Date: `2026-03-20`
- Step: `STEP-0151`
- Status: `completed`

## Goal

- define the smallest next experiment that can test whether missing synchronization after architectural timer sysreg writes prevents timer assertion

## Decision

The next implementation step is bounded to:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- add instruction barriers after architectural timer control and timer-value sysreg writes
- leave source selection, GIC wiring, and scheduler behavior unchanged

## Why This Step

- `STEP-0148` proved that neither timer source choice between virtual and physical non-secure restores dispatch
- `STEP-0150` proved that explicit PPI configuration still does not restore dispatch
- the remaining narrow common path is the timer sysreg write sequence in:
  - `hal_gtimerSetPhysicalControl()`
  - `hal_gtimerSetPhysicalTimer()`
  - `hal_gtimerSetVirtualControl()`
  - `hal_gtimerSetVirtualTimer()`

## Explicitly Deferred

- timer-source policy changes
- broader GIC refactoring
- timer-driver redesign
- real-hardware-only conclusions

## Acceptance Criteria

- the next implementation patch stays local to `hal/aarch64/aarch64.h`
- the experiment changes only timer-write synchronization
- the result will show whether explicit barriers restore timer dispatch on the generic fast lane

## Selected Next Step

- implement the architectural-timer write-barrier experiment and rerun both QEMU lanes
