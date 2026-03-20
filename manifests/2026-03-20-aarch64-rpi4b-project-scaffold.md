# Manifest: Initial Pi 4 Project Scaffold

- Date: `2026-03-20`
- Step: `STEP-0099`
- Result: `completed`

## Scope

- add the first Pi 4 project-local scaffold on top of `aarch64a53-generic`
- keep target reuse in the existing generic subfamily
- validate the new project through a no-hardware build

## Upstream Repositories

### `phoenix-rtos-project`

- Commit: `736b3f3`

## Files

- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/board_config.h`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`

## Validation

- refreshed the VM-local copied buildroot in `phoenix-dev`
- first attempted:
  `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host project image`
- the narrower command exposed that project image assembly requires the core artifacts and that the inherited generic `user.plo` expected a generated `/etc/system.dtb`
- added a project-local `user.plo.yaml` override that omits the generic-QEMU DTB blob
- reran:
  `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host core project image`

## Validation Evidence

- the Pi 4 scaffold now builds successfully in `phoenix-dev`
- the project produces:
  - `_boot/aarch64a53-generic-rpi4b/plo.elf`
  - `_boot/aarch64a53-generic-rpi4b/loader.disk`
- the new project remains layered on the existing generic target rather than requiring a new cross-repo target family/subfamily matrix

## Notes

- `board_config.h` seeds the Pi 4 project with the PL011 UART0 MMIO base `0xfe201000`
- the project currently seeds `PL011_TTY_CLOCK` with the documented UART0 default `48 MHz`; re-verify if the eventual boot configuration overrides `init_uart_clock`
- the project-local `user.plo.yaml` intentionally omits the generic-QEMU `system.dtb` blob because the Pi 4 path will need a firmware-facing DTB strategy rather than a QEMU-generated one

## Selected Next Step

- define the first Pi 4 firmware-facing boot-staging step
