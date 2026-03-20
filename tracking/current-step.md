# Current Step

## Metadata

- Step ID: `STEP-0158`
- Title: Implement timer-IRQ Group 1 experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether moving only the selected timer IRQ to Group 1 restores a readable enabled state and begins to unblock timer delivery on the current fast lanes

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- add a minimal interrupt-group helper
- set only the selected timer IRQ to Group 1 before enabling it
- preserve the timer IRQ state trace after handler registration
- keep timer-source policy, GIC configuration, and retry logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader timer redesign
- broader GIC policy changes
- interrupt-group policy changes
- secure-physical timer support
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane still reaches timer-handler registration and timer arming
- the generic lane exposes whether the selected timer IRQ moves to an enabled Group 1 state
- the experiment remains local to `hal/aarch64/interrupts_gicv2.c`
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to `hal/aarch64/interrupts_gicv2.c` and only changes group policy for the selected timer IRQ
- Build:
  rebuild the affected generic and Pi 4 project lanes in `phoenix-dev`
- Emulator:
  rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gic-timer-state-experiment.md`

## Notes

- Risks:
  avoid widening a bounded timer-only group experiment into a full interrupt-group rewrite before the timer path is proven
- Dependencies:
  completed `STEP-0157` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether the selected timer IRQ becomes Group 1 and enabled, and whether dispatch starts appearing
