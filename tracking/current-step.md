# Current Step

## Metadata

- Step ID: `STEP-0029`
- Title: Define the first generic AArch64 timer backend skeleton step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the first narrow generic AArch64 timer backend skeleton step now that the common `gtimer` helper layer exists

## Scope

In scope:

- inspect the current `gtimer` helper layer, timer hook, and DTB source-selection API after `STEP-0028`
- choose the smallest backend-skeleton responsibility to introduce next
- select the exact touched files for that backend-skeleton step
- keep this as a planning and scoping step only

Out of scope:

- adding a new QEMU target
- implementing the selected skeleton step itself
- changing the active timer backend for any target
- implementing the full common generic timer runtime backend itself
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `hal/aarch64/Makefile`
- likely a new common AArch64 timer-backend source file
- possibly a backend-local header
- tracking files and manifest updates for the chosen next step

## Acceptance Criteria

- the first generic AArch64 timer backend skeleton step is explicitly scoped with exact touched files, rationale, validation command, and success criteria
- the selected skeleton step is narrow enough to implement and validate in one controlled follow-up session

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-common-gtimer-helpers.md`

## Notes

- Risks:
  the next step must not jump directly to a full backend or skip over backend-state definition
- Dependencies:
  completed common `gtimer` helper step from `STEP-0028`
- User-visible control point before next step:
  the next code change should introduce only the smallest backend skeleton on top of the helper layer
