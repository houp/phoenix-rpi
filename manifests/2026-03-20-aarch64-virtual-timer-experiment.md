# Manifest: Virtual-First Timer-Source Experiment

- Date: `2026-03-20`
- Step: `STEP-0148`
- Status: `completed`

## Goal

- determine whether selecting the virtual architectural timer instead of the physical non-secure timer causes timer IRQ dispatch on the current fast lanes

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `bf100d04`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`

Changed only `dtb_chooseTimerSource()` so the virtual timer is preferred before the physical non-secure timer when both are available.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0148-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0148`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - changed to:
     - `gtimer: source virtual irq 27`
   - still reached:
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

- switching from the physical non-secure timer to the virtual timer does not restore timer dispatch on the generic fast lane
- the missing interrupt is therefore not explained by the current choice between those two DTB-exposed architectural timer sources
- the next bounded code clue is now in GIC configuration: the generic AArch64 GIC path configures SPIs at init time but does not explicitly configure PPIs such as timer IRQs 27 and 30

## Selected Next Step

- scope the first GIC PPI-configuration experiment for the timer IRQ path
