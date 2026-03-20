# Current Step

## Metadata

- Step ID: `STEP-0141`
- Title: Scope kernel sleep / wakeup programming visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next kernel-side diagnostic step that can prove whether the stalled retry path reaches sleep enqueue and wakeup programming in `proc/threads.c`

## Scope

In scope:

- inspect the common sleep path around:
  - `proc_threadNanoSleep()`
  - `_proc_threadSleepAbs()`
  - `_threads_programWakeup()`
- pick the minimum visibility point that can distinguish:
  - never enqueued for sleep
  - enqueued but wakeup not programmed
  - wakeup programmed but interrupt not delivered
- keep the next implementation step localized to one kernel subsystem
- preserve the current generic and Pi 4 QEMU validation lanes

Out of scope:

- broad timer backend refactoring
- immediate service-order workarounds
- changing `pl011-tty` retry semantics
- changing scheduler policy
- adding real-hardware-only diagnostics
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `sources/phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/proc/threads.c`
- manifests and tracking updates for the scope decision

## Acceptance Criteria

- the next implementation step is narrowed to one specific kernel visibility point
- the decision is justified against the current runtime evidence from `STEP-0140`
- the resulting scope avoids broad timer or scheduler churn

## Validation Plan

- Review:
  inspect the current common sleep / wake path and record the smallest justified visibility point
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-retry-wake-visibility.md`

## Notes

- Risks:
  avoid jumping prematurely from a proven sleep-stall symptom into a broad timer rewrite
- Dependencies:
  completed `STEP-0140` wake-return visibility result
- User-visible control point before next step:
  after this scope step lands, the next implementation patch should add only the minimum kernel-side marker needed to prove whether sleep enqueue and wakeup programming happen at runtime
