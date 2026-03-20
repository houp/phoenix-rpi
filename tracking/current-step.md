# Current Step

## Metadata

- Step ID: `STEP-0166`
- Title: Implement filtered Pi 4 loader call visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add narrowly filtered `plo` call-path visibility so the Pi 4 `raspi4b` lane shows whether it blocks before opening `user.plo`, while reading it, or while executing its first command after the official firmware DTB replaced the old stub

## Scope

In scope:

- change only `plo/cmds/call.c`
- add tightly filtered markers for:
  - successful `phfs_open()`
  - successful magic check
  - each script line immediately before `cmd_parse()`
- rebuild and rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b` with the official firmware DTB
- update manifests and docs with the exact new boundary

Out of scope:

- PHFS internals
- `plo/cmds/kernel.c`
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

- `plo/cmds/call.c`
- Pi 4 QEMU loader-script boundary notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the only upstream source change is in `plo/cmds/call.c`
- the generic `virt` lane still reaches the current working console-ready band
- the Pi 4 lane now exposes a narrower boundary inside `call ram0 user.plo`

## Validation Plan

- Review:
  confirm the markers are tightly filtered and limited to `cmd_call()`
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
  `manifests/2026-03-20-aarch64-rpi4b-official-dtb-validation.md`

## Notes

- Risks:
  avoid turning this into broad loader tracing or changing call semantics
- Dependencies:
  completed `STEP-0165` call-visibility scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from the exact new Pi 4 marker boundary, not from speculation about later kernel state
