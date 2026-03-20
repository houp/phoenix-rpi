# Manifest: Kernel Name-Service Visibility for `devfs`

- Date: `2026-03-20`
- Step: `STEP-0138`
- Status: `completed`

## Goal

- determine whether the blocked retry path re-enters kernel name lookup after `devfs` registers or whether it never gets that far

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `70e561a0`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/proc/name.c`

Added tightly filtered markers only for:

- `/` registration
- `devfs` registration
- `lookup("devfs", ...)` branch selection

The markers do not trace unrelated names.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0138-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0138`
- QEMU `10.2.2` for both runtime lanes

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `pl011-tty: tty0 lookup`
     - `create_dev: lookup devfs`
     - `name: devfs no root`
     - `create_dev: lookup done`
     - `pl011-tty: tty0 lookup retry`
     - `name: register devfs`
     - `dummyfs: devfs registered`
     - `dummyfs: devfs initialized`
   - did not reach before timeout:
     - any second `create_dev: lookup devfs`
     - `name: devfs cache hit`
     - `name: devfs root query`

2. Pi 4 DTB-backed `raspi4b`

   - remained unchanged at:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`
   - did not expose visible new kernel-side `name:` markers in this boot slice

## Conclusion

- the first `lookup("devfs")` is a no-root fast failure
- `devfs` does register later
- the retry path still never re-enters kernel name lookup after the first `usleep(100000)` site
- the next bounded target is therefore the post-`usleep()` wake-return path inside the `pl011-tty` helper

## Selected Next Step

- add a raw marker immediately after the retry-loop `usleep(100000)` returns so the fast lane can distinguish sleep-stall from second-lookup blocking
