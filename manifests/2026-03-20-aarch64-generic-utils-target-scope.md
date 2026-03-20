# Manifest: Generic AArch64 Utils Target Scope

- Date: `2026-03-20`
- Step: `STEP-0072`
- Result: `completed`

## Scope

- inspect the existing `phoenix-rtos-utils` target file for `aarch64a53-zynqmp`
- decide whether a generic counterpart can remain board-agnostic
- choose the next smallest repo-local userspace unblock step

## Upstream Repositories

- none

## Findings

- `phoenix-rtos-utils/_targets/Makefile.aarch64a53-zynqmp` contains only:
  `DEFAULT_COMPONENTS := psh`
- this target file is board-agnostic, unlike the current `phoenix-rtos-devices` target file
- `phoenix-rtos-utils` therefore remains a safer next unblock than `phoenix-rtos-devices` or `libphoenix`

## Selected Fix

- add `phoenix-rtos-utils/_targets/Makefile.aarch64a53-generic` with the same `psh` default component set

## Notes

- this keeps the generic-lane work in small repo-local slices
- after `phoenix-rtos-utils`, the next deliberate decision point should be whether to add a minimal generic-device target or to tackle `libphoenix` generic reboot support first

## Selected Next Step

- add the generic AArch64 target makefile to `phoenix-rtos-utils` and validate that repo on the generic target
