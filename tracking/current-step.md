# Current Step

## Metadata

- Step ID: `STEP-0211`
- Title: Implement the Pi 4 local prescaler experiment
- Status: `planned`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add one bounded Pi 4-only local prescaler write and compare the resulting
  local pending and dispatch evidence against the `STEP-0209` baseline

## Scope

In scope:

- add a Pi 4-only local prescaler value hook
- write `ARM_LOCAL_PRESCALER = 39768216U` once during local controller setup
- keep the existing local route-enable and local pending readback intact
- validate the Pi 4 A72 patched lane and use a generic build as a common-code
  guardrail

Out of scope:

- scheduler or VM changes
- broad interrupt-controller redesign
- Pi 5 or RP1 work

## Expected Repositories

- coordination repo
- `phoenix-rtos-kernel`
- `phoenix-rtos-project`
- coordination repo

## Expected Files Or Subsystems

- Pi 4 timer registration evidence after the restore
- Circle local-interrupt reference paths
- `external/circle/lib/sysinit.cpp`
- `external/circle/include/circle/bcm2836.h`
- `sources/phoenix-rtos-kernel/hal/aarch64/interrupts_gicv2.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/generic/config.h`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
- completed Pi 4 local-route-enable evidence
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the Pi 4 lane clearly reports whether the prescaler write changes the local
  pending or dispatch evidence
- the generic build guardrail remains healthy if common code is touched
- the result narrows the next move to one concrete follow-up on the same seam

## Validation Plan

- Review:
  inspect that the change stays bounded to the Pi 4 local prescaler path
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb RPI4B_QEMU_MEMORY_SIZE=80000000 TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run the Pi 4 A72 `raspi4b` lane and compare:
    - prescaler trace
    - `gtimer: local pending`
    - `gic: timer dispatch`
- Hardware:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-local-prescaler-scope.md`

## Notes

- Risks:
  do not mix the prescaler write with any additional local-controller changes
- Dependencies:
  completed `STEP-0210` local prescaler scope
- Source reminder:
  official Raspberry Pi kernel DTS files on `rpi-6.19.y` and `rpi-7.0.y` are currently identical for Pi 4 and keep `memory@0` bootloader-filled plus `stdout-path` on `serial1` (aux UART); Raspberry Pi documentation also confirms that firmware applies overlays and `dtparam`s before handing the merged DTB to the OS; this step specifically targets the root memory-node cell layout, not UART alias handling
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should depend only on whether
  the prescaler write changes the current local pending or dispatch evidence
