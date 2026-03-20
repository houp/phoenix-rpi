# Manifest: Kernel Sleep / Wakeup Visibility

- Date: `2026-03-20`
- Step: `STEP-0142`
- Status: `completed`

## Goal

- determine whether the blocked retry path reaches sleep enqueue, wakeup programming, and timer-interrupt delivery inside the common thread manager

## Upstream Repository

### `phoenix-rtos-kernel`

- Commit: `7343b4f8`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/proc/threads.c`

Added tightly filtered, one-time kernel console markers for the first relative `100000` us sleep:

- `threads: nsleep enter`
- `threads: wakeup programmed`
- `threads: timer irq`

The patch does not change timing, retry, or scheduler behavior.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0142-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0142`
- QEMU `10.2.2`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/phoenix-buildroots/phoenix-step0132/_boot/aarch64a53-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - reached:
     - `pl011-tty: tty0 lookup retry`
     - `threads: nsleep enter`
     - `threads: wakeup programmed`
     - later `name: register devfs`
     - later `dummyfs: devfs registered`
     - later `dummyfs: devfs initialized`
   - did not reach before timeout:
     - `threads: timer irq`
     - `pl011-tty: tty0 wake`

2. Pi 4 DTB-backed `raspi4b`

   - remained unchanged at:
     - loader startup
     - `pl011-tty: started`
     - `pl011-tty: register tty0`
     - `pl011-tty: tty0 lookup`
     - `pl011-tty: tty0 lookup retry`
   - did not expose visible new `threads:` markers in this boot slice

## Conclusion

- on the generic fast lane, the blocked retry path reaches `proc_threadNanoSleep()` and the kernel programs a wakeup deadline
- no timer interrupt reaches `threads_timeintr()` before timeout on that lane
- the next bounded blocker is therefore no longer sleep enqueue or wakeup programming; it is the common AArch64 timer source / IRQ-delivery path after wakeup programming
- the Pi 4 DTB-backed lane still does not expose the new kernel markers, so the generic lane remains the authoritative fast diagnostic lane for this blocker

## Selected Next Step

- scope the smallest common AArch64 timer source / IRQ visibility step before attempting a timer fix
