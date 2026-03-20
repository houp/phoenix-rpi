# Current Step

## Metadata

- Step ID: `STEP-0071`
- Title: Add generic AArch64 filesystem target makefile
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- apply the smallest repo-local change that starts removing the temporary generic-QEMU build-lane workarounds after the first visible kernel-banner milestone

## Scope

In scope:

- add `_targets/Makefile.aarch64a53-generic` in `phoenix-rtos-filesystems`
- keep the default component set board-agnostic and aligned with the existing `aarch64a53-zynqmp` file
- validate the `phoenix-rtos-filesystems` repo directly for the generic target in `phoenix-dev`
- record the result and stop before unblocking the next repo

Out of scope:

- changes in `phoenix-rtos-utils`, `phoenix-rtos-devices`, or `libphoenix`
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `libphoenix/arch/aarch64/reboot.c`
- `phoenix-rtos-filesystems/_targets/`
- `docs/status.md`
- tracking files and manifest updates for this step
- direct generic-target build output from `phoenix-rtos-filesystems`

## Acceptance Criteria

- `phoenix-rtos-filesystems` exposes a generic AArch64 target file
- the generic target file keeps only board-agnostic components
- the repo validates directly on `TARGET=aarch64a53-generic-qemu`

## Validation Plan

- Review:
  inspect the existing `aarch64a53-zynqmp` target file and keep the generic variant aligned
- Build:
  validate `phoenix-rtos-filesystems` directly for the generic target in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-userspace-unblock-scope.md`

## Notes

- Risks:
  the result must stay as one repo-local unblock step and must not silently turn into multi-repo implementation work
- Dependencies:
  completed implementation step `STEP-0070`
- User-visible control point before next step:
  after this repo-local unblock lands, the next slice should be the next smallest generic userspace blocker
