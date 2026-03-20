# Manifest: Generic `plo` EL3 GIC Group-Initialization Experiment

- Date: `2026-03-20`
- Step: `STEP-0160`
- Status: `completed`

## Goal

- determine whether generic `plo` EL3 GIC initialization is enough to make the later kernel-side timer IRQ state move away from `grp 0 en 0`

## Upstream Repository

### `plo`

- Commit: `f2afba1`

## Changes

Updated:

- `sources/plo/hal/aarch64/generic/interrupts.c`

Added EL3-aware generic GICv2 initialization that:

- places interrupts in Group 1 when running in EL3
- uses secure-mode CPU-interface control values compatible with later non-secure handoff
- preserves the previous non-EL3 generic path

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0160-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0160`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - improved from the previous blocker to:
     - `gic: timer handler set grp 0 en 1`
     - `gic: timer dispatch`
     - `threads: timer irq`
     - `pl011-tty: tty0 wake`
     - `pl011-tty: tty0 ready`
     - `pl011-tty: console ready`
     - visible kernel feature and VM startup logs
     - `dummyfs: initialized`

2. Pi 4 DTB-backed `raspi4b`

   - remained unchanged at:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`

## Conclusion

- generic `plo` EL3 GIC initialization is sufficient to unblock the shared timer-wakeup path on the generic fast lane
- the generic lane now reaches successful `/dev/tty0` and `/dev/console` registration and proceeds into visible kernel startup
- the Pi 4 DTB-backed lane remains unchanged, so the next bounded clue is not the generic EL3 fix itself but whether the `raspi4b` QEMU path reaches the loader in a different exception level or with different secure-state assumptions

## Selected Next Step

- scope generic `plo` entry-EL visibility on the Pi 4 lane
