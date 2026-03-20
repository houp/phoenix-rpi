# Manifest: Pi 4 Timer Group Override

- Date: `2026-03-20`
- Step: `STEP-0206`
- Status: `completed`

## Goal

- determine whether the visible Group 0 vs Group 1 timer-registration
  difference is the last remaining blocker between the working generic lane and
  the Pi 4 A72 patched lane

## Implementation

- added a default `TIMER_IRQ_GROUP` override hook in
  `sources/phoenix-rtos-kernel/hal/aarch64/generic/config.h`
- changed common AArch64 GICv2 timer-handler registration in
  `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
  to use that board-overridable group value
- forced `TIMER_IRQ_GROUP 0U` only on the Pi 4 A72 lane in
  `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`

## Validation

### Generic `virt` guardrail lane

- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - `qemu-system-aarch64 -machine virt,secure=on,gic-version=2 -cpu cortex-a53 -smp 1 -m 1G -serial mon:stdio -serial null -display none -kernel _boot/aarch64a53-generic-qemu/plo.elf -device loader,file=_boot/aarch64a53-generic-qemu/loader.disk,addr=0x48000000,force-raw=on`
- Key evidence:
  - `gic: timer handler set grp 0 en 1`
  - `gtimer: pending 1`
  - `gtimer: ppi pending 0`
  - `gic: timer dispatch`
  - `threads: timer irq`

### Pi 4 A72 patched lane

- Build:
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - `$HOME/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G -nographic -monitor none -kernel _boot/aarch64a72-generic-rpi4b/plo.elf -device loader,file=_boot/aarch64a72-generic-rpi4b/rpi4b/loader.disk,addr=0x48000000,force-raw=on`
- Key evidence:
  - `gic: timer handler set grp 0 en 1`
  - `gtimer: pending 0`
  - `gtimer: ppi pending 0`
  - `gtimer: post 2000 us ctl 0x5 ...`
  - no `gic: timer dispatch`
  - no `threads: timer irq`

## Result

- forcing the Pi 4 timer IRQ into Group 0 changes the visible registration
  state, but it does not restore timer dispatch
- the Group 0 vs Group 1 difference is therefore ruled out as the last missing
  variable
- the next step should restore the Pi 4 board-config baseline before testing a
  new runtime variable

## Next Step

- remove the temporary Pi 4 `TIMER_IRQ_GROUP 0U` override so the next
  experiment starts from the validated pre-step baseline
