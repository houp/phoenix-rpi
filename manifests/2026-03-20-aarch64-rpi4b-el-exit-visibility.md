# Manifest: Assembly-Side Pi 4 EL-Exit Visibility

- Date: `2026-03-20`
- Step: `STEP-0172`
- Status: `completed`

## Goal

- split the Pi 4 official-DTB silence after `hal: jump exit el1` without widening beyond `plo/hal/aarch64/generic/_init.S`

## Upstream Repository

### `plo`

- Commit: `a2e266c`

## Changes

Updated:

- `sources/plo/hal/aarch64/generic/_init.S`

Added tiny raw UART markers in `hal_exitToEL1()`:

- `A` at assembly handoff entry
- `3` immediately before the EL3 `eret`
- `2` immediately before the EL2 `eret`
- `1` immediately before the EL1 direct branch

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0172-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0172`
- QEMU `10.2.2`
- Pi 4 DTB source:
  - `https://github.com/raspberrypi/firmware`
  - commit `63ad7e7980b030cb4649ecedf2255c9226e5a1e8`
  - `boot/bcm2711-rpi-4-b.dtb`

Build validation:

- `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
- `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`

Runtime validation:

1. Generic `virt`

   - now shows:
     - `A3`
   - and then immediately reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`

2. Pi 4 DTB-backed `raspi4b`

   - now shows:
     - `A3`
   - and still never reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`
   - important new clue:
     - the Pi 4 lane prints multiple repeated assembly markers:
       - `AAA333`
       - then another `A3`

## Conclusion

- the Pi 4 official-DTB lane reaches the EL3 transfer point itself, not just the C-side call into `hal_exitToEL1()`
- the current Pi 4 boundary is now after the EL3 `eret`, either in the first kernel instructions after the transfer or in a multi-core handoff interaction
- the repeated `AAA333` / `A3` markers strongly suggest that multiple cores are taking the same generic loader EL3 handoff path on the Pi 4 lane
- the next bounded step should verify whether the same multi-core handoff pattern reproduces on the generic `virt` lane with `-smp 4` before changing handoff semantics

## Selected Next Step

- validate generic `virt` multi-core loader handoff behavior with `-smp 4`
