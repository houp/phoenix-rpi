# Manifest: Earliest Generic AArch64 Kernel-Entry Visibility

- Date: `2026-03-20`
- Step: `STEP-0177`
- Status: `completed`

## Goal

- determine whether the Pi 4 lane reaches generic AArch64 kernel `_start` after the single loader-side `A3` transfer marker

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `624a08e8`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/generic/config.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/_init.S`

Added:

- generic kernel inclusion of project `board_config.h`
- a default fallback `PL011_TTY_BASE`
- a tiny raw PL011 `uart_putc` macro in kernel `_init.S`
- a single earliest-entry marker:
  - `K` at generic kernel `_start`

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0177-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0177`
- QEMU `10.2.2`
- Pi 4 DTB source:
  - `https://github.com/raspberrypi/firmware`
  - commit `63ad7e7980b030cb4649ecedf2255c9226e5a1e8`
  - `boot/bcm2711-rpi-4-b.dtb`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt -smp 1`

   - shows:
     - `A3`
     - `K`
   - then reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`
     - later kernel and user-space startup logs

2. Pi 4 `raspi4b -smp 4`

   - shows:
     - `A3`
     - `K`
   - still does not reach:
     - `Phoenix-RTOS microkernel v. 3.3.1`

## Conclusion

- the Pi 4 lane definitely reaches generic kernel `_start`
- the remaining Pi 4 failure is now strictly after kernel entry and before the first visible kernel banner
- the next bounded split should stay in `hal/aarch64/_init.S` and divide the first early-init region into:
  - failure before the A53-specific system-register block
  - or failure inside / after that block

## Selected Next Step

- scope the first post-entry early-init split around the A53-specific kernel setup block
