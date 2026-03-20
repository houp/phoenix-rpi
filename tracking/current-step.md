# Current Step

## Metadata

- Step ID: `STEP-0020`
- Title: Add AArch64 generic timer source selection helpers
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- codify the first AArch64 generic timer source-selection policy in the DTB API so a future common timer backend can consume an explicit source decision instead of re-implementing selection logic

## Scope

In scope:

- update `phoenix-rtos-kernel/hal/aarch64/dtb.h`
- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- add a small AArch64 generic timer source-selection API
- encode the current common EL1 policy:
  - prefer non-secure physical timer
  - fall back to virtual timer
  - do not select secure or hypervisor timer in this path
- validate that the existing `aarch64a53-zynqmp-qemu` build still succeeds

Out of scope:

- adding a new QEMU target
- implementing the generic timer backend itself
- adding PL011 console code
- Raspberry Pi-specific code

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-kernel`

## Expected Files Or Subsystems

- `hal/aarch64/dtb.c`
- `hal/aarch64/dtb.h`
- copied-buildroot AArch64 validation workflow
- tracking files and manifest updates after validation

## Acceptance Criteria

- the DTB API exposes an explicit selected generic timer source and IRQ for common AArch64 use
- the source-selection policy is documented in code and remains small
- `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project` still succeeds inside `phoenix-dev` using the copied buildroot

## Validation Plan

- Build:
  refresh the copied buildroot, then run the existing `aarch64a53-zynqmp-qemu` build path in `phoenix-dev`
- Emulator:
  local `virt` timer DT node already inspected; confirm the chosen selection order matches the currently parsed timer metadata layout
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-timer-step-scope.md`

## Notes

- Risks:
  this is still a preparatory step and does not yet prove the generic timer backend itself
- Dependencies:
  completed timer planning step from `STEP-0019`
- User-visible control point before next step:
  present the exact DTB API change, validation command, and resulting commit before moving into the common timer backend implementation
