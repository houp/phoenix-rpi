# Manifest: Pi 4 DTB Propagation Into Kernel Payload

- Date: `2026-03-20`
- Step: `STEP-0109`
- Status: `completed`
- Upstream repository: `sources/phoenix-rtos-project`
- Upstream commit: `43f929d` (`project: propagate rpi4b dtb into loader payload`)

## Goal

- propagate a supplied Pi 4 DTB into the kernel-visible payload path as `system.dtb` while keeping default no-DTB builds green

## Changes

In `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`:

- resolve the optional Pi 4 DTB source once at project-script load time
- export `RPI4B_HAVE_DTB` only when a DTB is actually supplied
- copy the supplied DTB into `${PREFIX_ROOTFS}/etc/system.dtb`
- remove stale `${PREFIX_ROOTFS}/etc/system.dtb` on no-DTB builds
- keep staging the same DTB into the firmware-visible boot tree as `bcm2711-rpi-4-b.dtb`

In `sources/phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`:

- add a conditional `blob {{ env.BOOT_DEVICE }} /etc/system.dtb ddr` entry guarded by `RPI4B_HAVE_DTB`

## Validation

Validation ran in `phoenix-dev` from the copied buildroot:

- prepare buildroot:
  - `./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-step0109`

- no-DTB compatibility build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
  - result:
    - build passed
    - `_fs/aarch64a53-generic-rpi4b/root/etc/system.dtb` was absent
    - generated `_build/aarch64a53-generic-rpi4b/plo-scripts/user.plo` had no `blob ... system.dtb` entry

- supplied-DTB build:
  - a synthetic DTB was generated with `dtc` from a minimal `raspberrypi,4-model-b` / `brcm,bcm2711` test DTS
  - build command:
    - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH="$tmpdir/rpi4b-test.dtb" TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
  - result:
    - build passed
    - build log showed `system.dtb` packed into `part_kernel.img`
    - `_build/aarch64a53-generic-rpi4b/plo-scripts/user.plo` contained:
      - `alias -r system.dtb ...`
      - `blob ram0 system.dtb ddr`
    - SHA-256 matched between:
      - `_fs/aarch64a53-generic-rpi4b/root/etc/system.dtb`
      - `_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb`

## Outcome

- the Pi 4 project now satisfies the generic AArch64 kernel contract of supplying `system.dtb` in the loader payload when a board DTB is available
- default no-DTB builds still succeed without forcing a DTB on every build
- the next board-local boot blocker is no longer DTB transport; it is the generic `plo` hardcoded QEMU UART/GIC address set
