# Manifest: Generic `plo` Entry-EL Visibility

- Date: `2026-03-20`
- Step: `STEP-0162`
- Status: `completed`

## Goal

- determine which exception level the generic loader actually runs at on the Pi 4 `raspi4b` QEMU lane, relative to the now-working generic `virt` lane

## Upstream Repository

### `plo`

- Commit: `65d4302`

## Changes

Updated:

- `sources/plo/hal/aarch64/generic/hal.c`

Added a small console-visible current-EL trace immediately after generic loader console initialization.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0162-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0162`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - now shows:
     - `hal: entry EL3`
   - still preserves the current working path through:
     - `gic: timer dispatch`
     - `threads: timer irq`
     - `pl011-tty: tty0 ready`
     - `pl011-tty: console ready`

2. Pi 4 DTB-backed `raspi4b`

   - now also shows:
     - `hal: entry EL3`
   - but still stops at:
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`

## Conclusion

- the Pi 4 `raspi4b` lane does reach generic `plo` in EL3, so the unchanged Pi 4 behavior is not caused by missing the loader's EL3 setup path
- the next strongest remaining clue is the current Pi 4 DTB input itself, which is only a 274-byte stub containing `compatible` and one memory bank
- the next bounded step should therefore validate the Pi 4 lane with an official Raspberry Pi firmware DTB before more code changes are made

## Selected Next Step

- scope Pi 4 validation with an official firmware DTB
