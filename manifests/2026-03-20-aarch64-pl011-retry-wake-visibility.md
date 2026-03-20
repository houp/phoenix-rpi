# Manifest: `pl011-tty` Retry Wake-Return Visibility

- Date: `2026-03-20`
- Step: `STEP-0140`
- Status: `completed`

## Goal

- determine whether the bounded `pl011-tty` retry path stalls inside `usleep(100000)` rather than on a second `lookup("devfs", ...)` call

## Upstream Repository

### `phoenix-rtos-devices`

- Commit: `382c0be`

## Changes

Updated:

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`

Added one raw UART marker immediately after the first retry-loop `usleep(100000)` call:

- `pl011-tty: tty0 wake`

No retry timing, control flow, or registration semantics were changed.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0140-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0140`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `pl011-tty: tty0 lookup retry`
     - later `name: register devfs`
     - later `dummyfs: devfs registered`
     - later `dummyfs: devfs initialized`
   - did not reach before timeout:
     - `pl011-tty: tty0 wake`
     - any second `create_dev: lookup devfs`

2. Pi 4 DTB-backed `raspi4b`

   - reached:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`
   - did not reach before timeout:
     - `pl011-tty: tty0 wake`

## Conclusion

- the first bounded retry path now demonstrably blocks inside `usleep(100000)` on both fast QEMU lanes
- the active early-boot blocker is therefore below `pl011-tty` retry control flow and inside the common sleep / timer wakeup path
- the next bounded diagnostic target should move into the kernel sleep enqueue and wakeup-programming path before changing service order or retry semantics

## Selected Next Step

- scope the smallest kernel-side sleep / wakeup programming visibility step in `proc/threads.c`
