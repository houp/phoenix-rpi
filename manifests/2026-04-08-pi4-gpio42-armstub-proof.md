# 2026-04-08: Pi 4 GPIO42 Armstub Proof

## Scope

Implement the smallest real-hardware-visible Pi 4 earliest-entry proof without
widening the boot path:

- keep the current custom Pi 4 armstub
- keep the current high-placement `plo` model
- add only a GPIO42 activity-LED proof at the very start of the armstub on the
  primary core

## Why This Step

The latest real Pi 4 result remained:

- black screen
- red power LED only
- brief initial firmware ACT blink, then ACT off
- no keyboard-visible reaction

The low-level survey now ranks GPIO42 as the cleanest next proof point:

- it is the Pi 4 ACT LED GPIO
- it does not depend on UART
- it does not depend on framebuffer
- it does not depend on GIC delivery

So the next bounded question is:

- does the custom Pi 4 armstub execute at all on the real board?

## Touched Repositories

- `phoenix-rtos-project`
- coordination repo

Touched upstream commit:

- `phoenix-rtos-project 30f9fcf` `project: add pi4 gpio42 armstub proof`

## Touched Files

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/phoenix-armstub8-rpi4.S`
- `scripts/verify-rpi4b-sdimg.sh`
- `docs/status.md`
- `docs/platforms/raspberry-pi-4.md`
- `docs/source-artifacts.md`
- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Implementation

Added a primary-core-only GPIO42 helper to the custom Pi 4 armstub:

- configure GPIO42 as output through `GPIO_GPFSEL4`
- drive GPIO42 high through `GPIO_GPSET1`
- perform this before the existing local-timer and GIC setup

Key constants used:

- `GPIO_GPFSEL4 = 0xfe200010`
- `GPIO_GPSET1 = 0xfe200020`
- GPIO42 output field shift `6`
- GPIO42 set bit `10`

This leaves the rest of the current Pi 4 handoff path intact.

## Validation

### 1. Pi 4 A72 build

Validated in `phoenix-dev` with:

```sh
cd /Users/witoldbolt/phoenix-rpi
./scripts/prepare-rpi4b-dtb.sh
limactl shell -y phoenix-dev -- /bin/bash -lc '
  set -euo pipefail
  cd /Users/witoldbolt/phoenix-rpi
  ./scripts/prepare-buildroot.sh --copy-components
  export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
  cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy
  export RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb
  export RPI4B_QEMU_MEMORY_SIZE=80000000
  TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
'
```

Result:

- build passed

### 2. Pi 4 HDMI QEMU smoke

Validated with:

```sh
/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh
```

Result:

- passed
- framebuffer `1024x768`
- expected text-console pixels present

### 3. Pi 4 shell continuity

The strict `qemu-shell-smoke.sh rpi4b` prompt gate remained flaky in this run,
but the captured serial log still proved the same meaningful boot continuity:

- `plo`
- kernel
- `dummyfs`
- `pl011-tty`
- `psh`
- injected `help`

So this step is accepted as non-regressing on the strongest available
no-hardware Pi 4 lane.

### 4. Exported SD image

Refreshed export path:

```sh
./scripts/assemble-rpi4b-bootfs.sh
./scripts/assemble-rpi4b-bootfs-img.sh
./scripts/assemble-rpi4b-sdimg.sh
./scripts/export-rpi4b-sdimg.sh
```

Exported artifact:

- `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`

Validated SHA-256:

- `96743999a6e4312971de8787b36ef1bdb9affbd769a1b218b0943df3e77f73c7`

## Expected Next Hardware Result

For the next real Pi 4 retry:

- if the ACT LED turns on and stays on after the initial firmware blink, the
  custom armstub executed and the remaining failure is later
- if the ACT LED stays off after the initial firmware blink, the failure is
  still before or inside the current earliest custom armstub path

## Next Recommended Step

Wait for the next real Pi 4 board retry on this GPIO42-proof image and choose
the next smallest step from the ACT LED plus HDMI result.
