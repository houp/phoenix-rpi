# Current Step

## Metadata

- Step ID: `STEP-0193`
- Title: Instrument the Pi 4 `_vm_init` / `_map_init` boundary
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- narrow the first post-banner Pi 4 kernel abort by instrumenting the `_vm_init` / `_map_init` path directly

## Scope

In scope:

- add the smallest kernel-side visibility needed around `_vm_init` and the early `_map_init` pool initialization path
- rerun the generic fast lane and the Pi 4 A72 lane
- capture whether Pi 4 exposes the pool-state values or a tighter sub-boundary inside `_map_init`
- update manifests and docs with the result

Out of scope:

- generic VM algorithm changes
- broad DTB parser rewrites
- real-device DTB or firmware-bundle changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/vm/vm.c`
- `sources/phoenix-rtos-kernel/vm/map.c`
- bounded `_vm_init` / `_map_init` visibility
- Pi 4 A72 runtime evidence inside the current `_map_init` failure region
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic fast lane still reaches the established boot band
- the Pi 4 A72 lane exposes at least one new bounded `_vm_init` or `_map_init` marker before the current abort
- the result narrows the next step to one concrete follow-up rather than more DTB speculation

## Validation Plan

- Review:
  inspect the visibility change for minimality and keep it inside the current `_vm_init` / `_map_init` boundary
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run the generic fast lane on `virt`
  - run `qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G` against the patched A72 Pi 4 bundle
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-qemu-dtb-memory-hypothesis.md`

## Notes

- Risks:
  keep the instrumentation narrow; do not widen into a broad VM fix before the live failing sub-boundary is measured
- Dependencies:
  completed `STEP-0192` QEMU DTB memory-fix hypothesis check
- Source reminder:
  official Raspberry Pi kernel DTS files on both `rpi-6.12.y` and `rpi-6.19.y` keep `memory@0` bootloader-filled and keep `stdout-path` on `serial1` (aux UART), so this step should focus on live kernel state rather than on naive alias-resolution changes
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should be one precise kernel follow-up from the measured `_vm_init` / `_map_init` state, not a broad Pi 4 refactor
