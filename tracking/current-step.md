# Current Step

## Metadata

- Step ID: `STEP-0144`
- Title: Instrument common AArch64 timer source / IRQ visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine which common AArch64 timer source and IRQ are selected and whether the first wakeup arming reaches the timer frontend before the missing interrupt

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- add tightly filtered, one-time markers for:
  - selected timer source
  - selected IRQ number
  - first wakeup arming
- keep timer policy and IRQ routing unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- timer-source policy changes
- broad GICv2 refactoring
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane exposes the selected common AArch64 timer source
- the generic lane exposes the selected common AArch64 timer IRQ
- the generic lane exposes that the first wakeup arming reaches `gtimer_timer.c`
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to `hal/aarch64/gtimer_timer.c` and only adds filtered timer markers
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
  `manifests/2026-03-20-aarch64-kernel-sleep-visibility.md`

## Notes

- Risks:
  avoid mixing timer-source visibility with timer-policy changes or speculative fixes
- Dependencies:
  completed `STEP-0143` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from concrete source / IRQ visibility rather than from timer-source guesses
