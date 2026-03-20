# Manifest: Pi 4 Root Memory DTB Cell Parsing

- Date: `2026-03-20`
- Step: `STEP-0194`
- Status: `completed`

## Goal

- generalize root memory-bank `reg` parsing so Pi 4 root memory banks are
  decoded using the DTB root `#address-cells` and `#size-cells`

## Upstream Repositories

### `phoenix-rtos-kernel`

- Commit: `8a05f2ad`

## Changes

Updated:

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`

Behavioral change:

- `dtb_parseMemory()` no longer hardcodes a 16-byte `<addr,size>` tuple layout
- root memory-bank parsing now uses the already tracked DTB root cell widths
- zero-sized memory banks are skipped instead of being materialized as invalid
  banks

The step stays intentionally narrow:

- no alias parsing changes
- no `/soc` work beyond what was already landed in `STEP-0191`
- no VM algorithm changes

## Validation

Environment:

- `phoenix-dev`
- copied buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- `PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"` exported
  explicitly for non-interactive Lima shells

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=/tmp/rpi4b-qemu-memtest.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt` lane

   - still reaches the established later boot band, including:
     - kernel banner
     - `vm: map init done`
     - timer dispatch
     - `pl011-tty: tty0 ready`
     - `pl011-tty: console ready`

2. Pi 4 A72 `raspi4b` lane with the official firmware DTB

   - still reaches:
     - kernel banner
     - `vm: enter`
     - `vm: page init done`
     - `vm: map init`
     - `map: enter`
     - `map: pool link`
     - `map: zero free`
   - still aborts inside `_map_init`

3. Pi 4 A72 `raspi4b` lane with a one-off QEMU-patched DTB

   - DTB patch used:
     - `fdtput -t x /tmp/rpi4b-qemu-memtest.dtb /memory@0 reg 0 0 80000000`
   - now moves past the previous abort point and reaches:
     - `vm: map init done`
     - `gtimer: source virtual irq 27`
     - `gic: timer handler set grp 1 en 1`
     - `pl011-tty: started`
     - `threads: wakeup programmed`
     - `dummyfs: devfs initialized`
   - then stalls before the first visible timer interrupt or tty wakeup in the
     observed timeout window

## Conclusion

- the root memory-bank parser fix is correct and necessary
- the earlier negative Pi 4 DTB memory experiment was invalid because Phoenix
  was not yet parsing the Pi 4 3-cell root memory `reg`
- direct `qemu-system-aarch64 -M raspi4b` still does not provide Raspberry Pi
  firmware-time DTB memory customization, so the official firmware DTB remains
  insufficient for this emulated lane unless it is patched first
- the live Pi 4 blocker has therefore split cleanly:
  - real code issue fixed in `hal/aarch64/dtb.c`
  - remaining QEMU usability issue is a missing DTB memory fix in the emulated
    input path

## Selected Next Step

- automate the QEMU-only Pi 4 DTB memory fix in the project build or test path
  so the A72 `raspi4b` lane no longer depends on manual `fdtput` surgery before
  the next runtime-focused step
