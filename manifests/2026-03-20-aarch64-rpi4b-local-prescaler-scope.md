# Manifest: Pi 4 Local Prescaler Scope

- Date: `2026-03-20`
- Step: `STEP-0210`
- Status: `completed`

## Goal

- select the smallest next local interrupt controller follow-up after proving
  that the route-enable write alone does not restore pending or dispatch on the
  Pi 4 A72 lane

## Evidence Reviewed

Current Pi 4 local-route-enable evidence:

- `gic: local timer route 0x2`
- `gic: timer handler set grp 1 en 1`
- `gtimer: pending 0`
- `gtimer: ppi pending 0`
- `gtimer: local pending 0x0`
- no `gic: timer dispatch`

Circle reference evidence:

- `external/circle/lib/sysinit.cpp`
  writes `ARM_LOCAL_PRESCALER = 39768216U` on Pi 4
- `external/circle/include/circle/bcm2836.h`
  defines `ARM_LOCAL_PRESCALER = ARM_LOCAL_BASE + 0x008`
- `external/circle/lib/interrupt.cpp`
  already matches the route-enable write used in `STEP-0209`

## Selected Next Experiment

- add one Pi 4-only local prescaler write:
  - set `ARM_LOCAL_PRESCALER` to `39768216U`
  - keep the existing route-enable write and local pending readback unchanged
  - validate whether that one extra local-block setting changes pending or
    dispatch behavior

## Why This Is The Right Next Step

- it changes one additional variable from the strongest current external
  reference
- it stays inside the same BCM2711 local interrupt controller block
- it reuses the existing local pending instrumentation, so the result should be
  directly comparable to `STEP-0209`

## Selected Implementation Shape

- keep the prescaler value board-configurable and Pi 4-only
- write it once during local controller initialization, not in the recurring
  wakeup path
- add one trace line so the experiment proves that the prescaler value was
  actually written

## Selected Next Step

- implement the bounded Pi 4 local prescaler experiment and validate it on the
  Pi 4 A72 patched lane with a generic build guardrail
