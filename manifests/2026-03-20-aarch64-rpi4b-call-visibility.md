# Manifest: Filtered Pi 4 Loader Call Visibility

- Date: `2026-03-20`
- Step: `STEP-0166`
- Status: `completed`

## Goal

- split the Pi 4 official-DTB loader boundary inside `call ram0 user.plo` without widening beyond `plo/cmds/call.c`

## Upstream Repository

### `plo`

- Commit: `5e4dbcf`

## Changes

Updated:

- `sources/plo/cmds/call.c`

Added tightly filtered `user.plo` visibility markers for:

- successful `phfs_open()`
- successful magic check
- each parsed script line immediately before `cmd_parse()`

The markers use raw `lib_printf()` output so they remain visible during pre-init even when `log_info()` echo is disabled.

## Validation

Environment:

- `phoenix-dev`
- copied buildroots:
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0166b-generic`
  - `/home/witoldbolt.guest/phoenix-buildroots/phoenix-step0166`
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
     - `call: opened user.plo on ram0`
     - `call: magic ok user.plo`
     - `call: exec alias -r phoenix-aarch64a53-generic.elf 0x1000 0x22910`
     - `call: exec kernel ram0`
     - `call: exec alias -r system.dtb 0x24000 0x100000`
     - `call: exec blob ram0 system.dtb ddr`
     - `call: exec alias -r dummyfs 0x124000 0xf488`
     - `call: exec app ram0 -x dummyfs;-N;devfs;-D ddr ddr`
     - `call: exec alias -r pl011-tty 0x134000 0xda60`
     - `call: exec app ram0 -x pl011-tty ddr ddr`
     - `call: exec alias -r psh 0x142000 0x30bf0`
     - `call: exec app ram0 -x psh ddr ddr`
     - `call: exec go!`
   - and still preserves the current working path through:
     - `Phoenix-RTOS microkernel v. 3.3.1`
     - `pl011-tty: tty0 ready`
     - `pl011-tty: console ready`

2. Pi 4 DTB-backed `raspi4b`

   - now also shows:
     - `call: opened user.plo on ram0`
     - `call: magic ok user.plo`
     - `call: exec alias -r phoenix-aarch64a53-generic.elf 0x1000 0x22910`
     - `call: exec kernel ram0`
     - `call: exec alias -r system.dtb 0x24000 0xdc35`
     - `call: exec blob ram0 system.dtb ddr`
     - `call: exec alias -r dummyfs 0x32000 0xf488`
     - `call: exec app ram0 -x dummyfs;-N;devfs;-D ddr ddr`
     - `call: exec alias -r pl011-tty 0x42000 0xda80`
     - `call: exec app ram0 -x pl011-tty ddr ddr`
     - `call: exec alias -r psh 0x50000 0x30bf0`
     - `call: exec app ram0 -x psh ddr ddr`
     - `call: exec go!`
   - but still never reaches:
     - `Phoenix-RTOS microkernel v. 3.3.1`
     - any kernel, timer, or user-space log

## Conclusion

- the Pi 4 official-DTB lane is no longer blocked in loader script opening, magic verification, line fetch, or any pre-`go!` user-script command
- `kernel ram0` returns successfully on Pi 4, because later script lines continue to execute
- the current Pi 4 boundary is now strictly post-`go!`, inside `cmd_go()`, `hal_done()`, `hal_cpuJump()`, or the immediate handoff after `hal_cpuJump()`
- the next bounded step should therefore target post-`go!` visibility only

## Selected Next Step

- scope the first post-`go!` Pi 4 handoff visibility step
