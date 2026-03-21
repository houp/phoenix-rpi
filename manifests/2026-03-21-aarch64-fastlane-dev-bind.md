# Manifest: Fast-Lane `/dev` Bind Fix

- Date: `2026-03-21`
- Step: `STEP-0254`
- Upstream repo:
  - `phoenix-rtos-project 1187e5a`

## Change

- stage `psh` aliases for `mkdir` and `bind` in:
  - `aarch64a53-generic-qemu`
  - `aarch64a72-generic-rpi4b`
- run those aliases before the final `psh` app:
  - `mkdir /dev`
  - `bind devfs /dev`

## Validation

- copied-buildroot refresh in `phoenix-dev`
- generic build:
  - `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- generic runtime:
  - `qemu-system-aarch64 -machine virt,secure=on,gic-version=2 ...`
- Pi 4 build:
  - `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
  - with `RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb`
  - with `RPI4B_QEMU_MEMORY_SIZE=80000000`
- Pi 4 runtime:
  - `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 ...`

## Result

- generic `virt`:
  - old blocker removed:
    `psh: tty open fail open -2` no longer appears
  - new prompt band reached:
    - `psh: tty ready`
    - `psh: tcsetpgrp`
    - `psh: readcmd`
    - `(psh)%`
- Pi 4 `raspi4b`:
  - same old blocker removed
  - same prompt band reached:
    - `psh: tty ready`
    - `psh: tcsetpgrp`
    - `psh: readcmd`
    - `(psh)%`

## Conclusion

- the first prompt-reaching shared fast lane now exists on both generic and Pi 4
  QEMU
- the next small step should be cleanup of now-obsolete console-path probe code
  before widening into new boot or hardware work
