# Current Step

## Metadata

- Step ID: `STEP-0010`
- Title: Define the first narrow generic AArch64 QEMU lane step
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define and bound the next smallest implementation step needed to start a non-Xilinx generic AArch64 QEMU path without widening prematurely into full runtime bring-up

## Scope

In scope:

- inspect the current `aarch64a53-zynqmp-qemu` project and script flow
- identify the minimum target, build, project, and HAL pieces required for a first generic `virt`-style AArch64 lane
- select a narrow first code step that can be validated without Raspberry Pi hardware

Out of scope:

- implementing Raspberry Pi-specific code
- broad AArch64 HAL rewrites
- DTB parser rework
- real-hardware validation

## Expected Repositories

- coordination repo
- likely `phoenix-rtos-build`
- likely `phoenix-rtos-project`
- possibly `phoenix-rtos-kernel`
- possibly `plo`

## Expected Files Or Subsystems

- QEMU target and project definitions
- AArch64 build and run scripts
- tracking files and planning manifests for the next executable step

## Acceptance Criteria

- the next generic AArch64 QEMU step is explicitly scoped with concrete touched files, build command, and success criteria
- the proposed step is small enough to implement and validate in one narrow follow-up session

## Validation Plan

- Build:
  not applicable for this planning step
- Emulator:
  not applicable for this planning step
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-platform-makefile-selection.md`

## Notes

- Risks:
  the generic AArch64 QEMU lane may require more runtime HAL work than the build metadata alone suggests, so the next code step must be chosen conservatively
- Dependencies:
  completed Makefile generalization step and a working AArch64 validation toolchain in `phoenix-dev`
- User-visible control point before next step:
  present the exact proposed next change set before moving into new upstream code
