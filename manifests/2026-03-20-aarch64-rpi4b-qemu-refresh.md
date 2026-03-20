# Manifest: Pi 4 QEMU Refresh In `phoenix-dev`

- Date: `2026-03-20`
- Step: `STEP-0114`
- Status: `completed`

## Goal

- replace the current Ubuntu-packaged QEMU-only limitation with a documented VM-local QEMU lane that can emulate `raspi4b`

## Source Findings

From the official QEMU site on `2026-03-20`:

- `https://www.qemu.org/`
  - latest stable release listed was `10.2.2`
  - release date shown was `2026-03-17`

- `https://www.qemu.org/docs/master/system/arm/raspi.html`
  - documents `raspi4b`
  - documents implemented devices including:
    - interrupt controller
    - PL011 and AUX serial ports
    - GPIO
    - SD/MMC host controller
    - mailbox
    - USB host
  - documents missing devices including:
    - `PWM`
    - `PCIE Root Port (raspi4b)`
    - `GENET Ethernet Controller (raspi4b)`

## Changes

In `phoenix-dev`:

- keep the Ubuntu-packaged `/usr/bin/qemu-system-aarch64` as fallback
- add a VM-local source-built QEMU:
  - version: `10.2.2`
  - path: `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64`
- build configuration:
  - `--target-list=aarch64-softmmu,arm-softmmu`
  - `--disable-werror`
  - `--disable-docs`
  - `--disable-tools`
  - `--disable-user`

## Validation

Validation ran in `phoenix-dev`.

- packaged QEMU baseline:
  - `/usr/bin/qemu-system-aarch64 --version`
  - result: `QEMU emulator version 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.13)`
  - `/usr/bin/qemu-system-aarch64 -machine help`
  - result:
    - `raspi2b` present
    - `raspi3b` present
    - `raspi4b` absent

- VM-local latest-stable QEMU:
  - `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 --version`
  - result: `QEMU emulator version 10.2.2`
  - `/home/witoldbolt.guest/tools/qemu-10.2.2/bin/qemu-system-aarch64 -machine help`
  - result:
    - `raspi4b` present
    - `raspi3b` present
    - `virt` present

- first Pi 4 smoke attempt:
  - command shape:
    - `-machine raspi4b`
    - `-cpu cortex-a72`
    - `-kernel .../rpi4b/kernel8.img`
    - `-device loader,file=.../rpi4b/loader.disk,addr=0x48000000,force-raw=on`
  - first run with `-smp 1`:
    - failed immediately
    - QEMU error: `Invalid SMP CPUs 1. The min CPUs supported by machine 'raspi4b' is 4`
  - rerun with `-smp 4` and `-serial mon:stdio`:
    - QEMU started
    - no serial output appeared before timeout
  - rerun with `-smp 4` and `-nographic -monitor none`:
    - QEMU started
    - no serial output appeared before timeout

## Outcome

- the environment blocker is removed: `phoenix-dev` now has a practical `raspi4b`-capable QEMU lane
- the packaged Ubuntu QEMU remains available as fallback for older generic `virt` work
- the first `raspi4b` smoke is now a real boot-path blocker rather than an environment blocker
- inference:
  the next bounded diagnostic step should focus on the earliest emulated Pi 4 boot-path mismatch, likely around direct-kernel load semantics, entry assumptions, or early UART visibility rather than QEMU versioning itself
