# Manifest: A72 Pi 4 QEMU Runtime Validation

- Date: `2026-03-20`
- Step: `STEP-0185`
- Status: `completed`

## Goal

- validate the first `aarch64a72-generic-rpi4b` runtime lane on Pi 4 QEMU and compare its earliest visible boundary with the current A53 diagnostic lane

## Changes

No code changes.

## Validation

Environment:

- `phoenix-dev`
- VM-local QEMU `10.2.2` at `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0185-a72`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0185-a53`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Staged A72 Pi 4 boot bundle review:

- `_boot/aarch64a72-generic-rpi4b/plo.elf`
- `_boot/aarch64a72-generic-rpi4b/rpi4b/kernel8.img`
- `_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk`
- `_boot/aarch64a72-generic-rpi4b/rpi4b/config.txt`
- `_boot/aarch64a72-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb`

Runtime validation:

1. A72 Pi 4 lane

   - command shape:
     - `-M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none`
     - `-kernel _boot/aarch64a72-generic-rpi4b/plo.elf`
     - `-device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on`
   - observed output reaches:
     - `call: exec alias -r phoenix-aarch64a72-generic.elf ...`
     - `go: enter`
     - `go: devs done`
     - `go: hal done`
     - `hal: jump exit el1`
     - `A3`
     - `KLM`
   - then times out

2. A53 Pi 4 comparison lane

   - command shape:
     - `-M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none`
     - `-kernel _boot/aarch64a53-generic-rpi4b/plo.elf`
     - `-device loader,file=_boot/aarch64a53-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on`
   - observed output reaches the same earliest boundary:
     - `A3`
     - `KLM`
   - then times out

Additional evidence:

- the A72 loader script alias confirms the new kernel artifact is actually selected:
  - `call: exec alias -r phoenix-aarch64a72-generic.elf ...`
- the loader identification string still says:
  - `hal: Cortex-A53 Generic`
  so A72 target identity is not yet reflected in that diagnostic text

## Conclusion

- the first A72-capable Pi 4 target does not change the current Pi 4 QEMU runtime boundary
- the live failure remains after loader EL handoff and after the early `_init.S` `KLM` markers
- the next smallest high-signal step is to split the remaining `_init.S` path between `_core_0_virtual` and the branch into `main()`

## Selected Next Step

- scope a narrow post-`KLM` early-kernel visibility patch in `hal/aarch64/_init.S`
