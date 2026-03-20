# Manifest: `create_dev()` `debug()` Diagnostic Negative Result

- Date: `2026-03-20`
- Step: `STEP-0127`
- Status: `completed`

## Goal

- determine whether bounded `libphoenix`-side `debug()` markers can expose progress inside `create_dev("/dev/tty0")`

## Changes

Experimented locally in:

- `sources/libphoenix/unistd/file.c`

The temporary probe added `debug()` markers around:

- `lookup("devfs", ...)`
- fallback `/dev` handling
- directory-create messages
- final device-node create message

The probe was validated and then reverted because it produced no visible signal.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0127-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0127`

Build validation:

- `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `RPI4B_DTB_PATH=... TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - kernel banner
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
   - did not reach:
     - any new `create_dev:` marker from the `debug()` probe

2. Pi 4 DTB-backed `raspi4b`

   - reached:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
   - did not reach:
     - any new `create_dev:` marker from the `debug()` probe

## Conclusion

- the `debug()` path is not a useful visibility mechanism for this boot window
- future agents should not assume that adding `debug()` calls inside `create_dev()` will produce actionable early-boot signal

## Selected Next Step

- move the diagnostics to the kernel syscall side so at least one lane can distinguish `lookup("devfs", ...)` progress from later message-path blocking
