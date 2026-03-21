# 2026-03-21: Pi 4 QEMU HDMI Text Console

## Step

- `STEP-0303`
- `Implement the first Pi 4 framebuffer text output in pl011-tty`

## Upstream Repositories

- `phoenix-rtos-kernel`
  - `276c0c4d` `aarch64: fix generic syspage offsets with graphics`
- `plo`
  - `a21a680` `aarch64: publish generic graphmode after syspage init`
- `phoenix-rtos-devices`
  - `83c9167` `pl011-tty: add generic framebuffer text output`

## Validation

Build in `phoenix-dev` copied buildroot:

```sh
./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
export PATH=/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin:$PATH
export RPI4B_DTB_PATH=/home/witoldbolt.guest/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb
export RPI4B_QEMU_MEMORY_SIZE=80000000
export LIBPHOENIX_DEVEL_MODE=y
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Smoke:

```sh
./scripts/qemu-shell-smoke.sh rpi4b
/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh
```

Observed validated result:

- serial shell smoke passes and reaches `help`
- Pi 4 HDMI smoke passes with:
  - framebuffer `1024x768`
  - black console background
  - white text pixels in the expected text row
- Re-verify:
  a later clean rerun kept the HDMI smoke passing but did not complete the
  expected `Available commands:` round trip on `./scripts/qemu-shell-smoke.sh rpi4b`;
  that follow-up mismatch is now tracked as the next bounded step rather than
  folded back into this milestone.

## Key Findings

- the first `pl011-tty` HDMI text attempt was blocked by zero `pctl_graphmode`
  data in userspace
- a bounded gdbstub session proved the immediate first cause was packed
  `graphmode_t` by-value passing in `plo`
- a second bounded gdbstub session proved the deeper lifecycle issue:
  `video_init()` runs before `syspage_init()` in `plo/main()`, so graphics
  metadata must be published only after `syspage_init()`
- after fixing both issues, the Pi 4 QEMU lane now renders text over HDMI using
  the existing Phoenix bitmap font path reused inside `pl011-tty`

## Follow-up

- refresh and export the Pi 4 SD-card artifact chain from this validated HDMI
  text baseline
