# Manifest: Kernel-Side `create_dev()` Syscall Diagnostics

- Date: `2026-03-20`
- Step: `STEP-0128`
- Status: `completed`

## Goal

- determine whether `create_dev("/dev/tty0")` blocks in the `lookup("devfs", ...)` path or later in the create-message path

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `5b9dd867`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/syscalls.c`

Added narrow kernel-side markers for:

- `lookup("devfs", ...)`
- `lookup("/dev", ...)`
- the final `mtCreate` message filtered to `tty0`

The diagnostics emit entry / completion markers around the targeted syscall handlers.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0128-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0128`

Build validation:

- `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `RPI4B_DTB_PATH=... TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - kernel banner
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `create_dev: lookup devfs`
     - `create_dev: lookup done`
   - did not reach:
     - `create_dev: send tty0`
     - `create_dev: send done`

2. Pi 4 DTB-backed `raspi4b`

   - reached:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
   - did not reach:
     - visible kernel-side `create_dev:` markers

## Conclusion

- on the generic lane, the first `create_dev("/dev/tty0")` call returns from `lookup("devfs", ...)` and then stops before the final `msgSend()` syscall is reached
- the live boundary is therefore narrower than previously believed: it is now between the successful lookup return and final `msgSend()` entry inside `create_dev()`
- on the Pi 4 DTB-backed lane, kernel-side marker visibility is still insufficient, so the generic lane remains the authoritative fast diagnostic source for this slice

## Selected Next Step

- add a bounded user-space probe that is visible on stdout and targets only the tiny post-lookup gap inside `create_dev()`
