# Current Step

## Metadata

- Step ID: `STEP-0021`
- Title: Define the first directly selectable common AArch64 timer backend step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define and bound the first directly selectable common AArch64 timer backend step now that timer-source selection is exposed explicitly by the DTB API

## Scope

In scope:

- inspect the current AArch64 timer preparation work after `STEP-0020`
- choose the first directly selectable common timer backend shape
- select the smallest exact touched-file set for that backend step
- keep this as a planning and scoping step only

Out of scope:

- adding a new QEMU target
- implementing the selected backend itself
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `hal/aarch64/dtb.c`
- `hal/aarch64/dtb.h`
- `hal/aarch64/aarch64.h`
- likely a new common AArch64 timer source file or Makefile hook
- tracking files and manifest updates for the chosen next step

## Acceptance Criteria

- the first directly selectable common AArch64 timer backend step is explicitly scoped with exact touched files, rationale, validation command, and success criteria
- the selected next step is narrow enough to implement and validate in one controlled follow-up session

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-dtb-timer-source-selection.md`

## Notes

- Risks:
  the next timer backend step is the first one that will add substantial new runtime logic outside the current ZynqMP path
- Dependencies:
  completed DTB timer-source selection step from `STEP-0020`
- User-visible control point before next step:
  present the exact selected backend step before introducing the first directly selectable common timer implementation
