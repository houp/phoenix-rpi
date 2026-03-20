# Manifest: Pi 4 DTB Propagation Into Kernel Payload Scope

- Date: `2026-03-20`
- Step: `STEP-0108`
- Status: `completed`

## Goal

- define the smallest project-local change that propagates a supplied Pi 4 board DTB into the kernel-visible payload as `system.dtb`

## Source Findings

From the generic AArch64 kernel:

- `sources/phoenix-rtos-kernel/hal/aarch64/hal.c`
  - `_hal_init()` resolves a syspage program named `system.dtb`
  - if `system.dtb` is missing, the kernel halts

From the current Pi 4 project-local files:

- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
  - loads the kernel and user-space programs
  - does not load `system.dtb`
- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
  - already knows the optional Pi 4 DTB source path for firmware staging
  - currently does not propagate that DTB into `${PREFIX_ROOTFS}/etc/system.dtb`

From the generic QEMU project:

- `sources/phoenix-rtos-project/_projects/aarch64a53-generic-qemu/build.project`
  - generates `${PREFIX_ROOTFS}/etc/system.dtb`
- `sources/phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
  - loads that payload with:
    `blob {{ env.BOOT_DEVICE }} /etc/system.dtb ddr`

## Selected Next Step

- keep the Pi 4 DTB source optional
- when a Pi 4 DTB is supplied, copy it into `${PREFIX_ROOTFS}/etc/system.dtb`
- add the `blob {{ env.BOOT_DEVICE }} /etc/system.dtb ddr` line back to the Pi 4 project-local `user.plo.yaml`
- keep the change project-local to `phoenix-rtos-project`

## Planned Validation For The Next Step

- build:
  - `TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host core project image`
- artifact inspection:
  - `_fs/aarch64a53-generic-rpi4b/root/etc/system.dtb`
  - generated Pi 4 `user.plo` script includes the `blob ... /etc/system.dtb ddr` entry
- compatibility check:
  - default no-DTB builds should remain green if the step is implemented conditionally
