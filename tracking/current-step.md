# Current Step

## Metadata

- Step ID: `STEP-0031`
- Title: Define the first backend behavior-helper step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the first narrow behavior-helper step on top of the generic AArch64 timer backend state layer

## Scope

In scope:

- inspect the new backend-state layer after `STEP-0030`
- choose the smallest behavior helper to add next
- select the exact touched files for that behavior-helper step
- keep this as a planning and scoping step only

Out of scope:

- adding a new QEMU target
- implementing the selected helper step itself
- changing the active timer backend for any target
- implementing the public generic `hal_timer*` entry points
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `hal/aarch64/gtimer_backend.h`
- `hal/aarch64/gtimer_backend.c`
- tracking files and manifest updates for the chosen next step

## Acceptance Criteria

- the first backend behavior-helper step is explicitly scoped with exact touched files, rationale, validation command, and success criteria
- the selected helper step is narrow enough to implement and validate in one controlled follow-up session

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gtimer-backend-state-skeleton.md`

## Notes

- Risks:
  the next step must stay helper-only and must not cross the boundary into active backend ownership
- Dependencies:
  completed backend-state step from `STEP-0030`
- User-visible control point before next step:
  the next code change should add only one narrow behavior helper on top of the backend state
