# Current Step

## Metadata

- Step ID: `STEP-0072`
- Title: Define generic AArch64 `phoenix-rtos-utils` target unblock step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the next smallest board-agnostic repo-local change that continues removing the temporary generic-QEMU build-lane workarounds after the filesystem target unblock

## Scope

In scope:

- inspect the existing `phoenix-rtos-utils` target file for `aarch64a53-zynqmp`
- confirm whether a generic counterpart can stay board-agnostic
- choose the smallest repo-local utils unblock step
- keep the step planning-only

Out of scope:

- implementation code in this planning step
- changes in `phoenix-rtos-devices` or `libphoenix`
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `phoenix-rtos-utils/_targets/`
- `docs/status.md`
- tracking files and manifest updates for this step
- target-file inventory findings from `phoenix-rtos-utils`

## Acceptance Criteria

- the result names the next smallest board-agnostic utils unblock step
- the result explains why `phoenix-rtos-utils` is preferred over devices or `libphoenix` at this point
- the step remains planning-only

## Validation Plan

- Review:
  inspect the existing `aarch64a53-zynqmp` utils target file and the current generic-lane blockers
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-filesystems-target.md`

## Notes

- Risks:
  the result must stay as one repo-local planning step and must not silently turn into multi-repo implementation work
- Dependencies:
  completed implementation step `STEP-0071`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected utils target unblock change
