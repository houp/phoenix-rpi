# Manifest: GIC Timer Registration / Dispatch Visibility

- Date: `2026-03-20`
- Step: `STEP-0146`
- Status: `completed`

## Goal

- determine whether the selected common AArch64 timer IRQ is actually registered in GICv2 and whether it is ever dispatched before control should reach `threads_timeintr()`

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `1384877b`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`

Added tightly filtered, one-time kernel console markers for:

- timer-handler registration in `hal_interruptsSetHandler()`
- first dispatch of `hal_timerIrq()`

The patch does not change timer policy, GIC routing, or scheduler behavior.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0146-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0146`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `gtimer: source physical-nonsecure irq 30`
     - `gic: timer handler set`
     - `threads: nsleep enter`
     - `gtimer: arm 1000 us`
     - `threads: wakeup programmed`
     - later `name: register devfs`
     - later `dummyfs: devfs registered`
     - later `dummyfs: devfs initialized`
   - did not reach before timeout:
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
   - did not expose visible new `gic:` markers in this boot slice

## Conclusion

- the selected timer IRQ is successfully registered in GICv2 on the generic fast lane
- that same IRQ is never dispatched before timeout
- the remaining bounded blocker is therefore no longer handler registration or thread-manager wake logic; it is the timer-source / interrupt-generation side before GIC dispatch
- the next smallest actionable experiment is to change the common timer-source preference order and see whether a different architectural timer source produces dispatch on the same fast lane

## Selected Next Step

- scope the first timer-source selection experiment in the common AArch64 DTB timer policy
