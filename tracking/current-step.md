# Current Step

## Metadata

- Step ID: `STEP-0150`
- Title: Implement GIC PPI-configuration experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether explicit PPI configuration during handler registration is enough to make the selected timer IRQ dispatch on the current fast lanes

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- apply `interrupts_setConf()` to non-SGI interrupts during handler registration
- keep the current no-retargeting rule for PPIs
- keep timer-source policy, timer backend, and retry logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader GIC refactoring
- timer-source policy changes
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

- the generic lane still reaches timer-handler registration
- the generic lane exposes whether `gic: timer dispatch` begins to appear after explicit PPI configuration
- the experiment remains local to interrupt configuration for non-SGI IRQs
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to `hal/aarch64/interrupts_gicv2.c` and only changes non-SGI interrupt configuration during registration
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
  `manifests/2026-03-20-aarch64-virtual-timer-experiment.md`

## Notes

- Risks:
  avoid widening a bounded PPI-configuration experiment into a broad GIC rewrite
- Dependencies:
  completed `STEP-0149` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether explicit PPI configuration restores the first timer dispatch
