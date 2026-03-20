# Manifest: First Generic Userspace Build-Unblock Scope

- Date: `2026-03-20`
- Step: `STEP-0070`
- Result: `completed`

## Scope

- inspect the remaining generic-target blockers after the first visible generic kernel-banner milestone
- choose the smallest first repo-local change that reduces the temporary generic-QEMU build-lane workarounds
- keep the step planning-only

## Upstream Repositories

- none

## Findings

- `phoenix-rtos-filesystems` exposes only `_targets/Makefile.aarch64a53-zynqmp`, but its contents are board-agnostic:
  `dummyfs libext2 libjffs2`
- `phoenix-rtos-utils` is also small, but user-visible progress still depends on filesystem availability
- `phoenix-rtos-devices` is not the right first unblock because its current `aarch64a53-zynqmp` target file is board-specific
- `libphoenix` generic reboot support is still needed later, but it is not the narrowest first repo-local unblock for the current `LIBPHOENIX_DEVEL_MODE=n` fast lane

## Selected Fix

- add `phoenix-rtos-filesystems/_targets/Makefile.aarch64a53-generic` with the same board-agnostic default component set as the current `aarch64a53-zynqmp` target file

## Notes

- this is preferred over touching `phoenix-rtos-devices` first because it starts unblocking the generic userspace lane without committing to a generic UART or board-device story yet
- after filesystems, the next likely repo-local unblocks are `phoenix-rtos-utils` and then a deliberate generic-device strategy

## Selected Next Step

- add the generic AArch64 target makefile to `phoenix-rtos-filesystems` and validate that repo on the generic target
