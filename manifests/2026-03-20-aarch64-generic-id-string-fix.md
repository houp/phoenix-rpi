# Manifest: Generic AArch64 Identification String Fix

- Date: `2026-03-20`
- Step: `STEP-0187`
- Status: `completed`

## Goal

- remove misleading A53-only identification text from the new A72 Pi 4 runtime lane

## Upstream Repositories

### `plo`

- Commit: `a1a4745`

### `phoenix-rtos-kernel`

- Commit: `c4264f07`

## Changes

Updated:

- `sources/plo/hal/aarch64/generic/config.h`
- `sources/plo/hal/aarch64/generic/hal.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/generic/config.h`

`plo` now follows the same `CPU_INFO` pattern already used by other loader HALs, and the generic AArch64 CPU string is target-aware:

- `__TARGET_AARCH64A53` -> `Cortex-A53 Generic`
- `__TARGET_AARCH64A72` -> `Cortex-A72 Generic`

The kernel-side generic AArch64 platform name is now target-aware too:

- `__TARGET_AARCH64A53` -> `AArch64 Cortex-A53 Generic`
- `__TARGET_AARCH64A72` -> `AArch64 Cortex-A72 Generic`

## Validation

Environment:

- `phoenix-dev`
- VM-local QEMU `10.2.2`
- copied buildroots under `/home/witoldbolt.guest/phoenix-buildroots`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. A53 Pi 4 lane still reports:
   - `hal: Cortex-A53 Generic`

2. A72 Pi 4 lane now reports:
   - `hal: Cortex-A72 Generic`

In both cases, the remaining boot boundary is still:

- `A3`
- `KLM`

## Conclusion

- the misleading loader identification string is fixed
- the new A72 lane is now distinguishable from the A53 diagnostic lane in runtime logs
- the underlying Pi 4 boot blocker is unchanged, so the next step should return to the already-scoped post-`KLM` early-kernel visibility split

## Selected Next Step

- implement the first post-`KLM` early-kernel visibility split in common AArch64 `_init.S`
