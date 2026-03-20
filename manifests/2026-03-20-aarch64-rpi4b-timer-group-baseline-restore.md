# Manifest: Pi 4 Timer Group Baseline Restore

- Date: `2026-03-20`
- Step: `STEP-0207`
- Status: `completed`

## Goal

- restore the Pi 4 A72 lane to the validated pre-`STEP-0206` timer-group
  baseline before testing any new timer-routing hypothesis

## Implementation

- removed the temporary `TIMER_IRQ_GROUP 0U` override from
  `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- kept the generic kernel `TIMER_IRQ_GROUP` override hook intact so future
  bounded experiments can still use it without reworking common code

## Validation

### Pi 4 A72 patched lane

- Build:
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - `$HOME/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none -kernel _boot/aarch64a72-generic-rpi4b/plo.elf -device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on`
- Key evidence:
  - `gic: timer handler set grp 1 en 1`
  - `gtimer: pending 0`
  - `gtimer: ppi pending 0`
  - `gtimer: post 2000 us ctl 0x5 ...`
  - no `gic: timer dispatch`

## Result

- the Pi 4 lane is back on the known post-`STEP-0197` physical-timer baseline
- the failed Group 0 override is no longer active, so the next experiment can
  cleanly test one new variable

## Next Step

- scope the smallest Pi 4 local-interrupt-routing experiment, using Circle's
  `ARM_LOCAL_TIMER_INT_CONTROL0` and `ARM_LOCAL_IRQ_PENDING0` references as the
  current best guide for the BCM2711 timer-to-GIC seam
