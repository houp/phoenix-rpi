# Manifest: Pi 4 QEMU DTB Memory Hook

- Date: `2026-03-20`
- Step: `STEP-0195`
- Status: `completed`

## Goal

- remove manual `fdtput` work from the Pi 4 A72 `raspi4b` QEMU lane by adding
  a narrowly scoped build-time hook for the missing firmware-populated
  `memory@0/reg`

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `fd762d8`

## Changes

Updated:

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/build.project`

Behavioral change:

- new optional environment variable:
  - `RPI4B_QEMU_MEMORY_SIZE`
- when that variable is set and a DTB source is present, the Pi 4 project build
  now patches both:
  - `/etc/system.dtb`
  - `_boot/.../rpi4b/bcm2711-rpi-4-b.dtb`
- the patch is limited to:
  - `fdtput -t x ... /memory@0 reg 0 0 ${RPI4B_QEMU_MEMORY_SIZE}`
- when the variable is not set, the default DTB staging path stays unchanged

## Validation

Environment:

- `phoenix-dev`
- copied buildroot:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy`
- VM-local QEMU:
  - `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64`

Build validation:

1. Default path unchanged

   - build command:
     - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
   - result:
     - build succeeds with no `RPI4B_QEMU_MEMORY_SIZE`

2. Automated QEMU-patched path

   - build command:
     - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
   - result:
     - build succeeds
   - DTB verification:
     - `fdtget _boot/aarch64a72-generic-rpi4b/rpi4b/bcm2711-rpi-4-b.dtb /memory@0 reg`
     - observed output:
       - `0 0 -2147483648`

Runtime validation:

- `timeout 30s $HOME/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none -kernel _boot/aarch64a72-generic-rpi4b/plo.elf -device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on`

Observed boundary:

- reaches:
  - `vm: map init done`
  - `gtimer: source virtual irq 27`
  - `gic: timer handler set grp 1 en 1`
  - `pl011-tty: started`
  - `threads: wakeup programmed`
  - `dummyfs: devfs initialized`
- then stalls before:
  - `gic: timer dispatch`
  - `threads: timer irq`
  - `pl011-tty: tty0 wake`

That matches the later boundary already proved with the manual patched-DTB run.

## Conclusion

- the Pi 4 A72 `raspi4b` QEMU lane no longer depends on manual DTB surgery
- the remaining blocker is now a later runtime issue after `dummyfs`
  initialization, not DTB memory population
- the next bounded investigation should target the Pi 4 patched-lane timer or
  wakeup path, because the current visible stall is after timer arming but
  before the first visible timer interrupt

## Selected Next Step

- scope the first Pi 4 patched-lane timer-source or interrupt-delivery
  experiment now that the emulated DTB input is stable and automated
