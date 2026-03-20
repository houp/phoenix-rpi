# Current Step

## Metadata

- Step ID: `STEP-0030`
- Title: Implement generic AArch64 timer backend state skeleton
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add a compiled generic AArch64 timer backend state layer that owns the selected timer source, IRQ, and frequency while preserving the active ZynqMP backend

## Scope

In scope:

- add `hal/aarch64/gtimer_backend.h`
- add `hal/aarch64/gtimer_backend.c`
- compile the backend-state layer in the current common AArch64 build
- preserve the existing ZynqMP timer backend and validate the existing `aarch64a53-zynqmp-qemu` build in `phoenix-dev`

Out of scope:

- adding a new QEMU target
- changing the active timer backend for any target
- implementing the public generic `hal_timer*` entry points
- implementing the full common generic timer runtime backend itself
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/Makefile`
- `hal/aarch64/gtimer_backend.h`
- `hal/aarch64/gtimer_backend.c`
- tracking files and manifest updates for this step

## Acceptance Criteria

- the common AArch64 build now compiles a backend-state layer for the future generic timer backend
- the backend-state layer owns the selected source, IRQ, and frequency initialization
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
  `manifests/2026-03-20-aarch64-gtimer-backend-skeleton-scope.md`

## Notes

- Risks:
  the step must stay state-only and must not quietly take over the active timer API
- Dependencies:
  completed scoping step from `STEP-0029`
- User-visible control point before next step:
  after this skeleton lands, the next step should add one small behavior slice on top of it rather than activating the whole backend at once
