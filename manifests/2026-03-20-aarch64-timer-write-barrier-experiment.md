# Manifest: Architectural-Timer Write-Barrier Experiment

- Date: `2026-03-20`
- Step: `STEP-0152`
- Status: `completed`

## Goal

- determine whether explicit instruction barriers after architectural timer sysreg writes are enough to make the selected timer IRQ dispatch on the current fast lanes

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `0bc22817`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/aarch64.h`

Added explicit `isb` synchronization after writes to:

- `cntp_ctl_el0`
- `cntp_tval_el0`
- `cntv_ctl_el0`
- `cntv_tval_el0`

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0152-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0152`
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

- explicit instruction barriers after architectural timer sysreg writes do not restore timer dispatch on the current generic fast lane
- the Pi 4 DTB-backed lane remains unchanged, so the failed barrier experiment does not move the shared boot boundary
- the next bounded clue is whether the selected timer actually reports an armed control state and a live timer value after wakeup programming

## Selected Next Step

- scope the first architectural-timer register-readback experiment
