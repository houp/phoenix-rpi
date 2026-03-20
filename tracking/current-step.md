# Current Step

## Metadata

- Step ID: `STEP-0170`
- Title: Implement filtered Pi 4 `hal_cpuJump()` visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add narrowly filtered generic AArch64 jump-path visibility so the Pi 4 official-DTB lane shows whether it blocks before entering `hal_exitToEL1()` or only once the assembly EL handoff begins

## Scope

In scope:

- change only `plo/hal/aarch64/generic/hal.c`
- add raw jump markers for:
  - missing-entry failure
  - entry to `hal_cpuJump()`
  - after `hal_interruptsDisableAll()`
  - immediately before `hal_exitToEL1()`
  - unexpected return from `hal_exitToEL1()`
- rebuild and rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b` with the official firmware DTB
- update manifests and docs with the exact new boundary

Out of scope:

- assembly changes in `_init.S`
- changing Pi 4 image layout
- changing DTB content or selection
- kernel-side changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- `plo`
- coordination repo

## Expected Files Or Subsystems

- `plo/hal/aarch64/generic/hal.c`
- Pi 4 QEMU jump-path boundary notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the only upstream source change is in `plo/hal/aarch64/generic/hal.c`
- the generic `virt` lane still reaches the kernel banner after the new `hal:` jump markers
- the Pi 4 lane now exposes a narrower jump-path boundary

## Validation Plan

- Review:
  confirm the markers are tightly filtered and limited to `hal_cpuJump()`
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
  `manifests/2026-03-20-aarch64-rpi4b-go-visibility.md`

## Notes

- Risks:
  avoid jumping into assembly or board-specific changes before the C-side jump path is exhausted
- Dependencies:
  completed `STEP-0169` jump-visibility scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from the exact `hal:` jump marker boundary, not from speculation about the assembly handoff
