# Manifest: AArch64 Generic QEMU Project Step Scope

- Date: `2026-03-20`
- Step: `STEP-0056`
- Result: `completed`

## Scope

- inspect the existing `phoenix-rtos-project` QEMU target patterns and identify the smallest runnable generic AArch64 project step
- stop before implementation code
- choose the first boot artifact layout and runtime command for QEMU `virt`

## Findings

- the first generic AArch64 QEMU project step needs these files:
  - `phoenix-rtos-project/_targets/aarch64a53/generic/build.project`
  - `phoenix-rtos-project/_targets/aarch64a53/generic/nvm.yaml`
  - `phoenix-rtos-project/_targets/aarch64a53/generic/preinit.plo.yaml`
  - `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
  - `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/build.project`
  - `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/board_config.h`
  - `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- the first boot medium should be a RAM-backed `loader.disk`, not flash and not virtio:
  - the new generic `plo` path already has `ram-storage`
  - it does not yet have a generic flash, SD, or virtio-block loader path
  - QEMU can preload a raw image directly into the configured `RAM_ADDR` region with `-device loader,file=...,addr=...`
- the first preinit script should therefore use `BOOT_DEVICE=ram0`
- the first user script should stay kernel-only (`kernel ram0`, then `go!`) so the project step does not get blocked on a user-space PL011 driver that does not exist yet
- the first runtime command should target:
  - `qemu-system-aarch64`
  - `-machine virt,secure=on,gic-version=2`
  - `-cpu cortex-a53`
  - `-smp 1`
  - `-kernel .../plo.elf`
  - `-device loader,file=.../loader.disk,addr=RAM_ADDR,force-raw=on`
- `secure=on` is required in the first runtime lane because the current generic `plo` startup is intentionally EL3-centric

## Why This Step Comes Before The Emulated Test Target

- the emulated test target needs a stable artifact layout and a stable QEMU launch command
- those do not exist until the first project entry point names:
  - the target directory
  - the project directory
  - the boot image names under `_boot/`
  - the exact QEMU invocation

## Selected Next Step

- implement the first `aarch64a53-generic-qemu` project entry point with the RAM-backed `loader.disk` boot path
