# Current Step

## Metadata

- Step ID: `STEP-0188`
- Title: Implement the first post-`KLM` early-kernel visibility split
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- split the remaining common AArch64 early-kernel path after the current `A3KLM` boundary without widening into C-side instrumentation yet

## Scope

In scope:

- add one raw UART marker at `_core_0_virtual`
- add one raw UART marker immediately before the branch to `main()`
- rerun the generic fast lane and the Pi 4 A72 lane
- capture whether Pi 4 stops before `_core_0_virtual`, before `main()`, or inside `main()`
- update manifests and docs with the result

Out of scope:

- C-side instrumentation
- non-visibility runtime changes
- new target scaffolding
- changing Pi 4 image layout
- changing DTB content
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `hal/aarch64/_init.S`
- Pi 4 A72 early-kernel markers after `KLM`
- generic fast-lane regression evidence
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic fast lane still boots with the new markers
- the Pi 4 A72 lane produces enough marker output to place the failure relative to `_core_0_virtual` and `main()`
- the result narrows the next step to one concrete follow-up rather than a broad early-kernel rewrite

## Validation Plan

- Review:
  inspect the `_init.S` marker placement for minimality
- Build:
  - `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-qemu ./phoenix-rtos-build/build.sh clean host core project image`
  - `LIBPHOENIX_DEVEL_MODE=n RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- Emulator:
  - run the generic fast lane on `virt`
  - run `qemu-system-aarch64 -M raspi4b -cpu cortex-a72 -smp 4 -m 2G` against the built A72 Pi 4 bundle
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-post-klm-visibility-scope.md`

## Notes

- Risks:
  keep the patch visibility-only; do not roll visibility forward into C code in the same step
- Dependencies:
  completed `STEP-0186` post-`KLM` visibility scoping
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should be either a first C-entry visibility patch or one specific early-init fix, not a broad Pi 4 refactor
