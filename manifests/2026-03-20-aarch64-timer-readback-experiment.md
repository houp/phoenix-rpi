# Manifest: Architectural-Timer Register-Readback Experiment

- Date: `2026-03-20`
- Step: `STEP-0154`
- Status: `completed`

## Goal

- determine whether the selected architectural timer actually reports an armed control state and live timer value after wakeup programming on the current fast lanes

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `99d2b628`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/gtimer_timer.c`

Added direct architectural-timer `tval` readback helpers and changed the one-time wakeup trace to print the post-arm control state and timer value.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0154-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0154`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `gtimer: source virtual irq 27`
     - `gic: timer handler set`
     - `threads: nsleep enter`
     - `gtimer: arm 1000 us ctl 0x1 tval 58836`
     - `threads: wakeup programmed`
     - `dummyfs: devfs initialized`
   - still did not reach before timeout:
     - `gic: timer dispatch`
     - `threads: timer irq`
     - `pl011-tty: tty0 wake`

2. Pi 4 DTB-backed `raspi4b`

   - remained unchanged at:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`

## Conclusion

- the selected architectural timer is genuinely armed on the generic fast lane: control reads back as enabled and `tval` reads back as a live non-zero countdown
- the missing boundary is therefore no longer timer programming or timer-write ordering
- the next bounded clue should move to GIC state for the selected timer IRQ, especially interrupt-group and enable readback after handler registration

## Selected Next Step

- scope the first GIC timer-state visibility step
