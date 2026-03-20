# Current Step

## Metadata

- Step ID: `STEP-0036`
- Title: Implement backend-state timer-register wrappers
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add backend-state wrappers for control access and relative timer programming through the selected architectural timer source

## Scope

In scope:

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add state-based wrappers for control register reads and writes
- add a state-based wrapper for relative timer programming in ticks
- validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- changing the active timer backend for any target
- implementing the public generic `hal_timer*` entry points
- implementing timer interrupt registration
- adding timer-arming policy or wakeup semantics
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/gtimer_backend.h`
- `hal/aarch64/gtimer_backend.c`
- tracking files and manifest updates for this step

## Acceptance Criteria

- the backend-state layer exposes state-keyed wrappers for control register access and relative timer programming
- the wrappers use the timer source stored in backend state rather than open-coded source dispatch at future call sites
- the wrappers do not introduce public `hal_timer*` integration or timer-arming policy
- the existing `aarch64a53-zynqmp-qemu` build still succeeds in `phoenix-dev`

## Validation Plan

- Build:
  refresh the copied buildroot and rebuild `TARGET=aarch64a53-zynqmp-qemu` with `./phoenix-rtos-build/build.sh clean host core project`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gtimer-register-wrapper-scope.md`

## Notes

- Risks:
  the step must not grow into timer-arming policy; it should stay as a thin backend-state wrapper layer only
- Dependencies:
  completed scoping step from `STEP-0035`
- User-visible control point before next step:
  after this step lands, the next slice can target timer-arming policy or interrupt registration, but not both at once
