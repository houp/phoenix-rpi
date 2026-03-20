# Current Step

## Metadata

- Step ID: `STEP-0148`
- Title: Implement virtual-first timer-source experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether selecting the virtual architectural timer instead of the physical non-secure timer causes timer IRQ dispatch on the current fast lanes

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- change only `dtb_chooseTimerSource()`
- prefer the virtual timer over the physical non-secure timer when both are available
- keep the current generic timer backend, GIC wiring, and retry logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader timer refactoring
- GIC routing changes
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

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane selects the virtual timer instead of the physical non-secure timer
- the generic lane exposes whether timer dispatch starts working with the alternate source
- the experiment remains local to timer-source selection policy
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to `hal/aarch64/dtb.c` and only changes timer-source preference order
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
  `manifests/2026-03-20-aarch64-gic-timer-visibility.md`

## Notes

- Risks:
  avoid turning a small timer-source experiment into a broad policy change without runtime evidence
- Dependencies:
  completed `STEP-0147` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether the alternate architectural timer source produces the first dispatch
