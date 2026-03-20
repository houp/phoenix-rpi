# Current Step

## Metadata

- Step ID: `STEP-0172`
- Title: Implement assembly-side Pi 4 EL-exit visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add narrowly filtered loader assembly visibility so the Pi 4 official-DTB lane shows whether it reaches the EL-specific transfer point inside `hal_exitToEL1()` before going silent

## Scope

In scope:

- change only `plo/hal/aarch64/generic/_init.S`
- add tiny raw UART markers:
  - at `hal_exitToEL1()` entry
  - immediately before the EL-specific transfer instruction
- rebuild and rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b` with the official firmware DTB
- update manifests and docs with the exact new boundary

Out of scope:

- kernel-side changes
- changing Pi 4 image layout
- changing DTB content or selection
- semantic EL-handoff changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `plo`
- coordination repo

## Expected Files Or Subsystems

- `plo/hal/aarch64/generic/_init.S`
- Pi 4 QEMU EL-exit boundary notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the only upstream source change is in `plo/hal/aarch64/generic/_init.S`
- the generic `virt` lane still reaches the kernel banner after the new assembly marker
- the Pi 4 lane now exposes a narrower assembly-side EL-exit boundary

## Validation Plan

- Review:
  confirm the markers are tightly filtered and limited to the assembly EL-exit path
- Build:
  rebuild:
  - `TARGET=aarch64a53-generic-qemu`
  - `RPI4B_DTB_PATH=$HOME/external/raspberrypi-firmware/boot/bcm2711-rpi-4-b.dtb TARGET=aarch64a53-generic-rpi4b`
- Emulator:
  rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-jump-visibility.md`

## Notes

- Risks:
  avoid widening into kernel instrumentation before the loader assembly boundary is explicit
- Dependencies:
  completed `STEP-0171` assembly-side EL-exit scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from the exact assembly marker boundary, not from speculation about later kernel behavior
