# Current Step

## Metadata

- Step ID: `STEP-0191`
- Title: Translate Pi 4 serial MMIO through DTB `/soc/ranges`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- fix the most likely immediate Pi 4 console blocker by translating serial MMIO addresses from DTB bus addresses to CPU-visible physical addresses

## Scope

In scope:

- add the smallest generic DTB address-translation support needed for Pi 4 serial nodes under `/soc`
- rerun the generic fast lane and the Pi 4 A72 lane
- capture whether Pi 4 now reaches successful PL011 mapping and the existing post-console markers
- update manifests and docs with the result

Out of scope:

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

- `hal/aarch64/hal.c`
- `hal/aarch64/dtb.c`
- generic DTB address translation for serial nodes
- Pi 4 A72 console-init markers after `KLM`
- generic fast-lane regression evidence
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic fast lane still boots with the new markers
- the generic fast lane still boots after the translation change
- the Pi 4 A72 lane moves past `console: pl011 init done` or yields a more precise later boundary
- the result narrows the next step to one concrete follow-up rather than a broad early-kernel rewrite

## Validation Plan

- Review:
  inspect the address-translation change for minimality and keep it limited to the current serial-console need
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
  `manifests/2026-03-20-aarch64-generic-console-init-visibility.md`

## Notes

- Risks:
  keep the translation narrowly scoped; do not widen into full generic `ranges` recursion or unrelated DTB cleanup in the same step
- Dependencies:
  completed `STEP-0190` generic console-init visibility split
- Architecture reminder:
  Raspberry Pi 4 Model B is based on BCM2711 with a quad-core Cortex-A72 CPU; treat `aarch64a53-generic-rpi4b` only as a temporary diagnostic lane and keep new target work centered on `aarch64a72-generic-rpi4b`
- User-visible control point before next step:
  after this step lands, the next bounded move should be either a follow-up DTB console fix or the next single early-kernel split, not a broad Pi 4 refactor
