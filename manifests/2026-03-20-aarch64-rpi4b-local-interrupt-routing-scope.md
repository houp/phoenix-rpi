# Manifest: Pi 4 Local Interrupt Routing Scope

- Date: `2026-03-20`
- Step: `STEP-0208`
- Status: `completed`

## Goal

- select the smallest next runtime experiment on the Pi 4 timer-to-GIC seam
  after the failed Group 0 test and the baseline restore

## Evidence Reviewed

Current restored Pi 4 A72 lane evidence:

- `gic: timer handler set grp 1 en 1`
- `gtimer: pending 0`
- `gtimer: ppi pending 0`
- `gtimer: post 2000 us ctl 0x5 ...`
- no `gic: timer dispatch`

Circle reference evidence:

- `external/circle/include/circle/bcm2711int.h`
  defines `ARM_IRQLOCAL0_CNTPNS = GIC_PPI(14)`, so the current Phoenix timer
  IRQ identity is already aligned with a known Pi 4 reference
- `external/circle/include/circle/bcm2836.h`
  defines `ARM_LOCAL_BASE = 0xFF800000`,
  `ARM_LOCAL_TIMER_INT_CONTROL0 = ARM_LOCAL_BASE + 0x040`, and
  `ARM_LOCAL_IRQ_PENDING0 = ARM_LOCAL_BASE + 0x060`
- `external/circle/lib/interrupt.cpp`
  enables `ARM_LOCAL_TIMER_INT_CONTROL0` bit `1` for
  `ARM_IRQLOCAL0_CNTPNS` and checks `ARM_LOCAL_IRQ_PENDING0` before
  dispatching the local timer path

Current Phoenix evidence:

- there are no references to `ARM_LOCAL_TIMER_INT_CONTROL0`,
  `ARM_LOCAL_IRQ_PENDING0`, or `0xFF800000` in the current
  `phoenix-rtos-kernel` tree

## Selected Next Experiment

- add one Pi 4-only local interrupt controller routing hook for the current
  non-secure physical timer path:
  - enable `ARM_LOCAL_TIMER_INT_CONTROL0` bit `1` for core 0 when the timer
    IRQ handler is registered
  - read back `ARM_LOCAL_IRQ_PENDING0` in the existing bounded timer probe so
    the experiment can distinguish “route still absent” from “route enabled but
    still not reaching GIC dispatch”

## Why This Is The Right Next Step

- it changes one concrete runtime variable suggested by the strongest current
  external reference
- it stays on the current timer-to-GIC seam
- it does not widen into scheduler, VM, DTB, or SMP work
- it produces directly comparable evidence even if it fails

## Selected Implementation Shape

- keep the hook board-configurable and Pi 4-only
- place the route-enable logic in the existing timer-handler registration path
  rather than adding a new subsystem
- keep the new local pending readback diagnostic-only

## Selected Next Step

- implement the bounded Pi 4 local interrupt routing experiment and validate it
  on the Pi 4 A72 patched lane, with the generic `virt` lane used as a
  guardrail if common code changes extend beyond the Pi 4-only path
