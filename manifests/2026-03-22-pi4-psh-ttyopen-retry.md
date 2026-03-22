# STEP-0356

## Title

Implement the smallest bounded `psh_ttyopen()` retry-policy refinement for the Pi 4 shell startup race

## Date

2026-03-22

## Repositories

- `phoenix-rtos-utils`
- coordination repo

## Change Summary

The shell startup path now uses named retry-policy constants instead of a fixed
five-attempt loop when opening the first console.

The change:

- introduces `PSH_TTYOPEN_RETRIES`
- introduces `PSH_TTYOPEN_RETRY_US`
- raises the default retry budget from `5 * 100 ms` to `20 * 100 ms`

The fix stays localized to `psh` startup policy and does not change:

- `/dev/console` usage
- `pl011-tty`
- the kernel namespace path
- xHCI, PCIe, or USB-host logic

## Files

- `sources/phoenix-rtos-utils/psh/pshapp/pshapp.c`

## Validation

Validated in `phoenix-dev` with a fresh copied-buildroot Pi 4 A72 build:

```sh
./scripts/prepare-buildroot.sh --copy-components
cd ~/phoenix-buildroots/phoenix-rtos-project-copy
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
export RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb
export RPI4B_QEMU_MEMORY_SIZE=80000000
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

QEMU results:

- `./scripts/qemu-shell-smoke.sh generic`
  - passed
- `./scripts/qemu-shell-smoke.sh rpi4b`
  - the live log now reaches:
    - `psh: tty ready`
    - `(psh)%`
    - `Available commands:`
  - the helper still remains noisy because stale kernel `create_dev` probes
    interleave with the prompt after the shell is already functional
- `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`
  - passed

## Result

The Pi 4 shell-side console-open race is now functionally mitigated: the shell
reaches a usable prompt on the current Pi 4 QEMU lane again.

## Upstream Commit

- `phoenix-rtos-utils afe54cf`

## Next Step

- remove the stale kernel `create_dev` debug probes that now interfere with the
  automated Pi 4 shell smoke output
