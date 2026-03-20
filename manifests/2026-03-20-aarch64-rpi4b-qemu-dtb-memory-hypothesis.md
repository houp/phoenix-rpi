# Manifest: Pi 4 QEMU DTB Memory-Fix Hypothesis

- Date: `2026-03-20`
- Step: `STEP-0192`
- Status: `completed`

## Goal

- validate whether the current Pi 4 `raspi4b` QEMU abort is caused only by the uncustomized firmware DTB memory node

## Upstream Repositories

- none

This was a bounded validation-only step. No source repository was changed.

## Experiment

Environment:

- `phoenix-dev`
- copied buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`

One-off DTB patch:

- copied the official Pi 4 firmware DTB to `/tmp/rpi4b-qemu-memtest.dtb`
- ran:
  - `fdtput -t x /tmp/rpi4b-qemu-memtest.dtb /memory@0 reg 0 0 80000000`
- verified the patched node decompiled as:
  - `memory@0 { reg = <0x00 0x00 0x80000000>; }`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=/tmp/rpi4b-qemu-memtest.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

- reran the standard Pi 4 A72 `raspi4b` QEMU lane with `-m 2G`

## Result

- negative

The patched-memory DTB reaches exactly the same boundary as the unmodified official DTB:

- `console: pl011 init done`
- `hal: console init done`
- `main: hal init done`
- `Phoenix-RTOS microkernel v. 3.3.1 ...`
- `Exception #37: Data Abort (EL1)`
- `pc=ffffffffc000b198`
- `lr=ffffffffc000b0cc`

## Additional Evidence From Official Raspberry Pi Kernel Sources

From `raspberrypi/linux` branch `rpi-6.12.y`:

- `arch/arm/boot/dts/broadcom/bcm2711-rpi.dtsi`
  - `memory@0` is explicitly commented as:
    - `Will be filled by the bootloader`
- `arch/arm/boot/dts/broadcom/bcm2711-rpi-4-b.dts`
  - `chosen { stdout-path = "serial1:115200n8"; }`
  - comment:
    - `8250 auxiliary UART instead of pl011`

These official source facts are more authoritative than decompiling the already-built DTB for structural intent.

## Conclusion

- a non-zero `memory@0/reg` value alone is not enough to move the current Pi 4 QEMU abort
- direct `raspi4b` QEMU is still missing at least one other firmware-time customization, or the live bug is elsewhere in later kernel bring-up
- the official DTS also confirms that naive `stdout-path` alias resolution would currently point Phoenix at the auxiliary UART, not the PL011 path Phoenix uses now

## Selected Next Step

- instrument the Pi 4 `_vm_init` / `_map_init` boundary directly so the next change is based on live kernel state rather than more DTB speculation
