# Current Step

## Metadata

- Step ID: `STEP-0164`
- Title: Validate Pi 4 QEMU lane with an official firmware DTB
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether replacing the current stub Pi 4 DTB with an official Raspberry Pi firmware DTB moves the `raspi4b` QEMU lane beyond the current `tty0 lookup retry` boundary

## Scope

In scope:

- acquire an official `bcm2711-rpi-4-b.dtb` from the Raspberry Pi firmware repository
- record the exact firmware repo revision used for the validation
- rebuild the Pi 4 project with `RPI4B_DTB_PATH` pointing to that DTB
- rerun the Pi 4 `raspi4b` QEMU lane
- update manifests and docs with the exact DTB source used

Out of scope:

- kernel or loader code changes
- any new interrupt-controller policy changes
- changing `pl011-tty` retry semantics
- changing scheduler policy
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- external Raspberry Pi firmware DTB source
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the exact firmware DTB source and revision are recorded
- the Pi 4 validation uses a real firmware DTB rather than the current stub
- the Pi 4 result shows whether a real DTB moves the lane beyond `tty0 lookup retry`

## Validation Plan

- Review:
  confirm the DTB comes from the official Raspberry Pi firmware repository and record the exact revision used
- Build:
  rebuild the Pi 4 project in `phoenix-dev` with `RPI4B_DTB_PATH` pointing to that DTB
- Emulator:
  rerun:
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-plo-entry-el-experiment.md`

## Notes

- Risks:
  avoid mixing input-quality validation with new code changes
- Dependencies:
  completed `STEP-0163` scope decision
- User-visible control point before next step:
  after this step lands, the next bounded move should come from whether the real Pi 4 DTB shifts the `raspi4b` lane or whether a different Pi 4-specific blocker remains
