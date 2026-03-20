# Current Step

## Metadata

- Step ID: `STEP-0154`
- Title: Implement architectural-timer register-readback experiment
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether the selected architectural timer actually reports an armed control state and live timer value after wakeup programming on the current fast lanes

## Scope

In scope:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- add minimal readback helpers for the selected architectural timer
- emit a one-time post-arm trace for control state and timer value
- keep timer-source policy, GIC configuration, and retry logic unchanged
- validate on the generic `virt` lane first, then on the Pi 4 DTB-backed `raspi4b` lane

Out of scope:

- broader timer redesign
- GIC policy changes
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

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane still reaches timer-handler registration and timer arming
- the generic lane exposes the selected timer control state and timer value after wakeup programming
- the experiment remains local to common AArch64 timer helper code
- neither QEMU lane regresses from current known-good boot output

## Validation Plan

- Review:
  confirm the patch stays localized to the common AArch64 timer helper path and only adds bounded readback visibility
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
  `manifests/2026-03-20-aarch64-timer-write-barrier-experiment.md`

## Notes

- Risks:
  avoid widening a bounded timer-state visibility experiment into a broader timer or interrupt redesign
- Dependencies:
  completed `STEP-0153` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether timer readback shows a failed arm state or a valid armed state without later IRQ delivery
