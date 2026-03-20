# Current Step

## Metadata

- Step ID: `STEP-0032`
- Title: Implement backend-state time conversion helpers
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the first backend-state behavior helpers for reading the selected timer count and converting it to microseconds

## Scope

In scope:

- update `hal/aarch64/gtimer_backend.h`
- update `hal/aarch64/gtimer_backend.c`
- add state-based count-reading and microsecond conversion helpers
- validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- changing the active timer backend for any target
- implementing the public generic `hal_timer*` entry points
- implementing timer interrupt registration or wakeup programming
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

- the backend-state layer exposes helpers for raw count reads, count-to-microseconds conversion, and current microsecond reads
- the helpers use the selected source and frequency stored in backend state
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
  `manifests/2026-03-20-aarch64-gtimer-backend-behavior-scope.md`

## Notes

- Risks:
  the step must stay read-only with respect to timer behavior and must not start programming timers yet
- Dependencies:
  completed scoping step from `STEP-0031`
- User-visible control point before next step:
  after this step lands, the next slice can target interrupt or wakeup behavior, but not both at once
