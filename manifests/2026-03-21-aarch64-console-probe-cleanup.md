# Manifest: Console Probe Cleanup

- Date: `2026-03-21`
- Step: `STEP-0256`
- Upstream repos:
  - `phoenix-rtos-utils c9d1504`
  - `libphoenix 118dea3`
  - `phoenix-rtos-kernel 08261996`

## Change

- remove the obsolete `/dev/console` investigation traces from:
  - `sources/phoenix-rtos-utils/psh/psh.c`
  - `sources/libphoenix/unistd/file.c`
  - `sources/phoenix-rtos-kernel/syscalls.c`
  - `sources/phoenix-rtos-kernel/posix/posix.c`
- keep the broader fast-lane boot markers that are still useful for later
  bring-up

## Validation

- copied-buildroot refresh in `phoenix-dev`
- generic build:
  - `TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- generic runtime:
  - `qemu-system-aarch64 -machine virt,secure=on,gic-version=2 ...`
  - verified in `/tmp/generic-cleanup.log`
- Pi 4 build:
  - `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
  - with `RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb`
  - with `RPI4B_QEMU_MEMORY_SIZE=80000000`
- Pi 4 runtime:
  - `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 ...`
  - verified in `/tmp/pi4-cleanup.log`

## Result

- generic `virt` still reaches:
  - `psh: tty ready`
  - `psh: readcmd`
  - `(psh)%`
- Pi 4 `raspi4b` still reaches the same prompt band
- the removed console-specific probe strings no longer appear in either log:
  - `syscalls: psh console lookup`
  - `posix: psh console open`
  - `psh: tty open fail`

## Conclusion

- the prompt-reaching fast lane now survives without the old console-path
  diagnostics
- the next smallest useful step is to validate one real interactive shell
  command on both QEMU lanes
