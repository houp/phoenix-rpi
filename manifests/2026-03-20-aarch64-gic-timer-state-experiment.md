# Manifest: GIC Timer-State Visibility Step

- Date: `2026-03-20`
- Step: `STEP-0156`
- Status: `completed`

## Goal

- determine whether the selected timer IRQ is enabled and in the expected interrupt group after handler registration on the current fast lanes

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `78a40e75`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`

Added a tightly filtered timer-specific GIC trace that prints the selected timer IRQ's interrupt-group bit and enable readback immediately after handler registration.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0156-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0156`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `gtimer: source virtual irq 27`
     - `gic: timer handler set grp 0 en 0`
     - `gtimer: arm 1000 us ctl 0x1 tval 58461`
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

- the selected timer IRQ still reads back as `grp 0 en 0` immediately after registration on the generic fast lane
- combined with the existing `plo` handoff path, which exits EL3 to EL1 non-secure, this strongly suggests the timer PPI is being left in the wrong security group, so the enable write never sticks
- the next bounded fix should therefore change only the selected timer IRQ to Group 1 before enabling it

## Selected Next Step

- scope the first timer-IRQ Group 1 experiment
