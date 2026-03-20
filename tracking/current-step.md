# Current Step

## Metadata

- Step ID: `STEP-0143`
- Title: Scope common AArch64 timer source / IRQ visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next diagnostic step that can prove which common AArch64 timer source and IRQ are being selected and armed before the missing timer interrupt should arrive

## Scope

In scope:

- inspect the common AArch64 timer path around:
  - `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
  - `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
  - `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- pick the minimum visibility point that can expose:
  - selected timer source
  - selected IRQ number
  - timer registration and first wakeup arming context
- keep the next implementation step localized to the common AArch64 timer code
- preserve the current generic and Pi 4 DTB-backed QEMU lanes

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

- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_backend.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`
- manifests and tracking updates for the scope decision

## Acceptance Criteria

- the next implementation step is narrowed to one specific timer-visibility point in the common AArch64 timer path
- the scope is justified against the current `STEP-0142` result
- the resulting step avoids mixing timer visibility with policy changes or fixes

## Validation Plan

- Review:
  inspect the common AArch64 timer path and record the smallest justified visibility point
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-kernel-sleep-visibility.md`

## Notes

- Risks:
  avoid turning a proven missing-timer-interrupt symptom into an unbounded timer-policy rewrite
- Dependencies:
  completed `STEP-0142` kernel sleep / wakeup visibility result
- User-visible control point before next step:
  after this scope step lands, the next implementation patch should expose the selected common AArch64 timer source and IRQ before any fix is attempted
