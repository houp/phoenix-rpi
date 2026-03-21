# 2026-03-21: implement the staged Pi 4 `plo` HDMI progress indicator

## Scope

- Step: `STEP-0297`
- Goal: improve early Pi 4 no-UART observability by replacing the plain
  framebuffer rectangle with a tiny staged progress indicator

## Repositories Touched

- `plo`
- coordination repo

Upstream commit:

- `plo f10aefd`

## Changes

Updated:

- `sources/plo/hal/aarch64/generic/video.c`
- `sources/plo/hal/aarch64/generic/hal.c`
- `scripts/qemu-rpi4b-hdmi-smoke.sh`

The new Pi 4 early HDMI behavior is:

- framebuffer still comes from the Raspberry Pi property mailbox path
- the background stays unchanged
- a small top-left panel now contains three square progress markers
- marker 0 lights when framebuffer setup succeeds
- marker 1 lights during late `hal_init()`
- marker 2 lights when `hal_cpuJump()` starts the kernel handoff

This is intentionally still an early loader visibility aid only:

- not a text console
- not a runtime framebuffer driver
- not a general graphics stack

## Validation

Rebuilt the Pi 4 lane in `phoenix-dev`:

```sh
./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
export PATH=/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin:$PATH
export RPI4B_DTB_PATH=/home/witoldbolt.guest/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb
export RPI4B_QEMU_MEMORY_SIZE=80000000
export LIBPHOENIX_DEVEL_MODE=n
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Validated with:

```sh
/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh
```

Observed framebuffer pixels:

- `(20, 20) -> (72, 72, 72)` panel background
- `(48, 48) -> (240, 240, 240)` stage 0 lit
- `(112, 48) -> (240, 240, 240)` stage 1 lit
- `(176, 48) -> (240, 240, 240)` stage 2 lit
- `(639, 479) -> (160, 96, 48)` background preserved

## Result

- the current Pi 4 no-UART path now communicates more than simple “alive”
- a first real board trial can now distinguish at least three loader phases
  from HDMI alone
- the next practical step is to refresh the host-visible Pi 4 SD image so the
  staged indicator is present on the actual board media

