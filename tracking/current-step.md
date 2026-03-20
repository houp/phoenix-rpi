# Current Step

## Metadata

- Step ID: `STEP-0195`
- Title: Automate the Pi 4 QEMU-only DTB memory fix
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- remove the manual `fdtput` dependency from the Pi 4 A72 `raspi4b` QEMU lane
  by adding a narrowly scoped QEMU-only DTB memory patch path in project build
  glue

## Scope

In scope:

- keep the kernel fix from `STEP-0194` unchanged
- add the smallest possible build-time hook for a Pi 4 QEMU-only DTB
  `memory@0/reg` patch
- keep the default real-device DTB path unchanged when that hook is not used
- validate that the automated QEMU-patched lane reaches the same later boundary
  as the manually patched DTB did
- update manifests and docs with the result

Out of scope:

- further kernel DTB parsing changes
- generic VM algorithm changes
- real-device DTB or firmware-bundle changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `phoenix-rtos-kernel`
- `phoenix-rtos-project`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/build.project`
- optional Pi 4 QEMU-only DTB patch hook
- Pi 4 A72 runtime evidence with no manual DTB surgery
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the default Pi 4 build path stays unchanged when the new hook is not enabled
- the automated Pi 4 QEMU-patched lane reaches at least the same later boundary
  already proved by the manual patched-DTB run
- the result removes manual DTB surgery from the fast emulated Pi 4 loop
- the next step narrows to one concrete runtime follow-up after the new
  automated QEMU lane is in place

## Validation Plan

- Review:
  inspect the build-glue change for minimality and keep it limited to an
  optional QEMU-only DTB memory patch path
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run `qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G` against
    the default A72 Pi 4 bundle as a no-hook regression check if needed
  - run the same lane against the automated-QEMU-memory-patched A72 Pi 4 bundle
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-root-memory-cell-parsing.md`

## Notes

- Risks:
  keep the fix narrow; do not mix QEMU-only DTB build patching with broader
  firmware-bundle or kernel-runtime work in the same step
- Dependencies:
  completed `STEP-0194` root memory DTB cell parsing
- Source reminder:
  official Raspberry Pi kernel DTS files on `rpi-6.19.y` and `rpi-7.0.y` are currently identical for Pi 4 and keep `memory@0` bootloader-filled plus `stdout-path` on `serial1` (aux UART); Raspberry Pi documentation also confirms that firmware applies overlays and `dtparam`s before handing the merged DTB to the OS; this step specifically targets the root memory-node cell layout, not UART alias handling
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should be one precise runtime
  follow-up from the automated Pi 4 QEMU lane, not a broad firmware or VM
  refactor
