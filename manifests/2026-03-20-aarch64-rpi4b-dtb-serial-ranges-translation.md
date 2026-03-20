# Manifest: Pi 4 DTB Serial `/soc/ranges` Translation

- Date: `2026-03-20`
- Step: `STEP-0191`
- Status: `completed`

## Goal

- fix the immediate Pi 4 console blocker by decoding `/soc` serial `reg` cells correctly and translating serial MMIO through `/soc/ranges`

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `05c5ed9a`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`

Added the smallest DTB parsing support needed for Pi 4 serial MMIO:

- root `#address-cells` and `#size-cells` tracking
- `/soc` `#address-cells`, `#size-cells`, and `ranges` parsing
- serial `reg` decoding through the parent `/soc` cell width instead of treating `<addr size>` as one 64-bit value
- `/soc/ranges` translation for parsed serial base addresses
- matching `/soc/ranges` translation for `stdout-path` values that encode a unit address directly

The scope was kept serial-only. This step does not generalize arbitrary nested bus translation, alias-style `stdout-path` resolution, or non-serial device address decoding.

## Validation

Environment:

- `phoenix-dev`
- copied buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- `PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"` exported explicitly for non-interactive Lima shells

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt` lane

   - still reaches:
     - `console: pl011 init done`
     - `hal: console init done`
     - `main: hal init done`
     - the established tty / console-ready boot band

2. Pi 4 A72 `raspi4b` lane

   - now reaches:
     - `console: pl011 init done`
     - `hal: console init done`
     - `main: hal init done`
     - `Phoenix-RTOS microkernel v. 3.3.1 ...`
   - then aborts with:
     - `Exception #37: Data Abort (EL1)`
     - `pc=ffffffffc000b198`
     - `lr=ffffffffc000b0cc`

Symbolization:

- `pc=ffffffffc000b198` -> `_map_init` at `phoenix-rtos-kernel/vm/map.c:1638`
- `lr=ffffffffc000b0cc` -> `_map_init` at `phoenix-rtos-kernel/vm/map.c:1624`

## Additional Evidence

From the official Raspberry Pi firmware DTB at `boot/bcm2711-rpi-4-b.dtb`:

- `serial@7e201000 { reg = <0x7e201000 0x200>; ... }`
- `/soc { ranges = <0x7e000000 0x00 0xfe000000 0x1800000 ...>; }`
- `memory@0 { reg = <0x00 0x00 0x00>; }`

From Raspberry Pi documentation:

- the firmware loader customizes the DTB before launching the kernel

## Conclusion

- the serial-address bug is fixed well enough for Pi 4 to pass kernel console initialization
- the next blocker is later in kernel startup, no longer in early serial MMIO resolution
- the official firmware DTB also contains a zeroed `memory@0` placeholder, which is a strong clue that direct `raspi4b` QEMU validation may still need a QEMU-side DTB memory fix because Raspberry Pi firmware is not in the loop there

## Selected Next Step

- validate the QEMU-only Pi 4 DTB memory-node hypothesis before widening into broader VM or memory-map debugging
