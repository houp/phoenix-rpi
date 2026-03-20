# Manifest: Pi 4 GIC PPI-State Scope

- Date: `2026-03-20`
- Step: `STEP-0203`
- Status: `completed`

## Goal

- select the smallest GIC-side follow-up after proving that the Pi 4 timer
  expires locally while the current pending probe still reports `0`

## Evidence Reviewed

Current Phoenix Pi 4 A72 patched-lane evidence:

- `gtimer: source physical-nonsecure irq 30`
- `gtimer: arm 1000 us ctl 0x1 tval ...`
- `gtimer: pending 0`
- `gtimer: post 2000 us ctl 0x5 tval ...`
- no `gic: timer dispatch`

Cross-check from Circle:

- Pi 4 non-secure physical timer IRQ is `GIC_PPI(14) = 30`
- Circle uses the physical counter path on Pi 4
- Circle confirms that Phoenix is already on the right timer and IRQ identity

Current open question:

- whether the current Phoenix pending helper is reading the wrong GIC view for a
  private timer interrupt

## Selected Next Experiment

- add one bounded timer-only readback of the GIC private pending state for the
  selected timer IRQ and compare it with the current `ISPENDR`-based readback

## Why This Is The Right Next Step

- it stays on the exact timer-to-GIC seam
- it does not widen into scheduler, VM, or broad interrupt-controller redesign
- it directly tests the most plausible narrow explanation left after `ctl 0x5`
  plus `pending 0`: the timer may be visible through the private pending-state
  path rather than the current register view
- the needed register offset already exists in Phoenix GIC code

## Selected Implementation Shape

- keep the change diagnostic-only
- emit at most one extra first-arm PPI-state line
- use the existing GIC timer probe path and compare:
  - current `ISPENDR`-based state
  - `PPISR`-based state for the same timer IRQ

## Selected Next Step

- implement the bounded private-pending-state readback experiment and validate
  it on the generic `virt` guardrail lane plus the Pi 4 A72 patched lane
