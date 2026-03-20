# Manifest: GIC PPI-Configuration Experiment

- Date: `2026-03-20`
- Step: `STEP-0150`
- Status: `completed`

## Goal

- determine whether explicit PPI configuration during handler registration is enough to make the selected timer IRQ dispatch on the current fast lanes

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `ad2018ea`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`

Applied `interrupts_setConf()` to all non-SGI interrupts during handler registration while preserving the existing no-retargeting rule for PPIs.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0150-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0150`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - still reached:
     - `gtimer: source virtual irq 27`
     - `gic: timer handler set`
     - `threads: nsleep enter`
     - `gtimer: arm 1000 us`
     - `threads: wakeup programmed`
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

- explicit PPI configuration during registration does not restore timer dispatch on the current generic fast lane
- the live boundary is now below source selection, below handler registration, and below simple PPI configuration
- the next smallest experiment should move into the architectural timer sysreg write path itself, where write-order or missing synchronization could still prevent timer assertion

## Selected Next Step

- scope the first architectural-timer write-barrier experiment
