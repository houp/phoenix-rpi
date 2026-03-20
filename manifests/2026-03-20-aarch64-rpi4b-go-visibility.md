# Manifest: Filtered Post-`go!` Pi 4 Visibility

- Date: `2026-03-20`
- Step: `STEP-0168`
- Status: `completed`

## Goal

- split the Pi 4 official-DTB silence after `call: exec go!` without widening beyond `plo/cmds/go.c`

## Upstream Repository

### `plo`

- Commit: `6f2cbf1`

## Changes

Updated:

- `sources/plo/cmds/go.c`

Added raw `go:` markers for:

- entry to `cmd_go()`
- after `devs_done()`
- after `hal_done()`
- immediately before `hal_cpuJump()`
- unexpected return from `hal_cpuJump()`

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0168-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0168`
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
     - `go: enter`
     - `go: devs done`
     - `go: hal done`
     - `go: jump`
   - and then immediately reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`

2. Pi 4 DTB-backed `raspi4b`

   - now also shows:
     - `go: enter`
     - `go: devs done`
     - `go: hal done`
     - `go: jump`
   - but still never reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`
     - any kernel, timer, or user-space log
   - and does not show:
     - `go: jump returned ...`

## Conclusion

- the Pi 4 official-DTB lane is no longer blocked in `cmd_go()` cleanup before the jump
- `devs_done()` and `hal_done()` both complete on Pi 4
- the current Pi 4 boundary is now strictly inside `hal_cpuJump()` or the immediate EL handoff after it calls `hal_exitToEL1()`
- the next bounded step should target `hal_cpuJump()` / EL-exit visibility only

## Selected Next Step

- scope the first Pi 4 `hal_cpuJump()` / EL-exit visibility step
