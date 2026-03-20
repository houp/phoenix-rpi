# Manifest: Timer-IRQ Group 1 Experiment

- Date: `2026-03-20`
- Step: `STEP-0158`
- Status: `completed`

## Goal

- determine whether moving only the selected timer IRQ to Group 1 from the kernel side restores an enabled timer IRQ state and begins to unblock dispatch

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `33f018a5`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`

Added a minimal `interrupts_setGroup()` helper and used it only for the selected timer IRQ before enabling it.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0158-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0158`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - still reached:
     - `gtimer: source virtual irq 27`
     - `gic: timer handler set grp 0 en 0`
     - `gtimer: arm 1000 us ctl 0x1 tval 59047`
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

- moving the selected timer IRQ to Group 1 from the kernel side does not take effect: the generic fast lane still reads back `grp 0 en 0`
- the next bounded boundary is therefore above the kernel, in the generic `plo` EL3 handoff path where secure interrupt-group state can still be established
- the smallest next fix should move to generic `plo` GIC initialization, not a broader kernel interrupt rewrite

## Selected Next Step

- scope the first generic `plo` EL3 GIC group-initialization experiment
