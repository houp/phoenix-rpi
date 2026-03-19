# Current Step

## Metadata

- Step ID: `STEP-0009`
- Title: Generalize top-level AArch64 platform Makefile selection
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- replace the hardwired `zynqmp` substring matching in the top-level AArch64 Makefiles with generic per-subfamily Makefile selection, while keeping the existing `aarch64a53-zynqmp` build behavior intact

## Scope

In scope:

- update `phoenix-rtos-kernel` AArch64 build glue to include the platform Makefile by `TARGET_SUBFAMILY` rather than matching the literal string `zynqmp`
- update `plo` AArch64 build glue the same way
- keep the existing `zynqmp` object selection local to the `zynqmp` platform Makefiles
- validate the unchanged `aarch64a53-zynqmp-qemu` build path in `phoenix-dev` using the copied buildroot lane

Out of scope:

- adding a new AArch64 target
- changing DTB parsing, timer logic, interrupt logic, or any runtime behavior
- adding Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- `plo`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/Makefile`
- `sources/phoenix-rtos-kernel/hal/aarch64/zynqmp/Makefile`
- `sources/plo/hal/aarch64/Makefile`
- `sources/plo/hal/aarch64/zynqmp/Makefile`
- copied-buildroot AArch64 validation workflow
- tracking files and manifest updates after validation

## Acceptance Criteria

- the top-level AArch64 Makefiles no longer rely on a literal `findstring zynqmp` check to select the platform Makefile
- the existing `aarch64a53-zynqmp` object set remains complete and builds without functional regressions in the build glue
- `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project` succeeds inside `phoenix-dev` using a refreshed copied buildroot
- the resulting upstream patches stay small, readable, and limited to build-glue generalization

## Validation Plan

- Build:
  refresh the copied buildroot, then run the existing `aarch64a53-zynqmp-qemu` build path in `phoenix-dev`
- Emulator:
  not required for this step beyond proving the build artifacts are produced for the existing QEMU target
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-toolchain-bootstrap.md`

## Notes

- Risks:
  the existing `aarch64a53-zynqmp-qemu` build may rely on implicit `zynqmp`-specific object placement in ways that make the smallest generalization slightly broader than the top-level Makefile diff suggests
  the copied buildroot must be refreshed before validation so the VM sees the edited upstream sources
- Dependencies:
  completed toolchain bootstrap step and a running `phoenix-dev` VM
- User-visible control point before next step:
  present the exact build command, result, and upstream commits before moving on to any DTB or Raspberry Pi-specific work
