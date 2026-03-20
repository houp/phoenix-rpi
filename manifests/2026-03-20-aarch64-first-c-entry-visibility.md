# Manifest: First C-Entry Visibility After `KLM`

- Date: `2026-03-20`
- Step: `STEP-0189`
- Status: `completed`

## Goal

- split the remaining Pi 4 early-kernel path at the first usable C-managed console points after `A3KLM`

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `960e607f`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/hal.c`
- `sources/phoenix-rtos-kernel/main.c`

Added visibility markers:

- `hal: console init done` immediately after `_hal_consoleInit()`
- `main: hal init done` immediately after `_hal_init()` returns

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0189-qemu`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0189-rpi4b`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt` lane

   - reaches both new markers:
     - `hal: console init done`
     - `main: hal init done`
   - continues into the existing boot band with kernel banner, timer, `pl011-tty`, and later startup logs

2. Pi 4 A72 lane

   - still reaches only:
     - `A3`
     - `KLM`
   - does not reach either new C-side marker

## Conclusion

- Pi 4 still fails before `_hal_consoleInit()` completes and before `_hal_init()` returns
- the current blocker window is now narrowed to:
  - the front part of `_hal_init()`
  - `_pmap_preinit()`
  - `_hal_platformInit()`
  - or `_hal_consoleInit()` before successful console enable

## Selected Next Step

- split the generic AArch64 console-init path, starting with visibility after `dtb_getConsoleSerial()` and `hal_pl011Init()`
