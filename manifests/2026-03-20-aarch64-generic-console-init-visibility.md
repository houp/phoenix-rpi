# Manifest: Generic Console-Init Visibility Split

- Date: `2026-03-20`
- Step: `STEP-0190`
- Status: `completed`

## Goal

- split the remaining Pi 4 early-kernel path inside generic console initialization

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `370c771f`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/generic/console.c`

Added an early direct PL011 probe:

- `console: pl011 init done` immediately after successful `hal_pl011Init()`

This marker intentionally bypasses the normal `console_common.enabled` path and writes directly through the mapped PL011 helper, so it can distinguish successful MMIO mapping from later console-enable work.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0190-qemu`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0190-rpi4b`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt` lane

   - reaches:
     - `console: pl011 init done`
     - `hal: console init done`
     - `main: hal init done`
   - still continues into the established boot band

2. Pi 4 A72 lane

   - still reaches only:
     - `A3`
     - `KLM`
   - does not reach:
     - `console: pl011 init done`
     - `hal: console init done`
     - `main: hal init done`

## Additional Evidence

From the official Raspberry Pi firmware DTB:

- `stdout-path = "serial0:115200n8"`
- `serial@7e201000`
- `/soc/ranges = <0x7e000000 0x00 0xfe000000 0x1800000 ...>`

Phoenix currently:

- parses serial `reg` values directly in `dtb_parseSerial()`
- does not apply parent `ranges` translation
- parses `stdout-path` only when it contains an `@...` unit address

## Conclusion

- Pi 4 still fails before successful PL011 mapping in generic console init
- the strongest concrete next fix is now DTB address handling, not another generic console probe
- the most likely immediate bug is that the Pi 4 serial node base is being kept as bus address `0x7e201000` instead of translated CPU-visible address `0xfe201000`

## Selected Next Step

- implement a minimal generic DTB address translation step for serial console MMIO on Pi 4, starting with `/soc/ranges`
