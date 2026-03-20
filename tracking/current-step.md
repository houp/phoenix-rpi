# Current Step

## Metadata

- Step ID: `STEP-0087`
- Title: Add `dummyfs` and `pl011-tty` to the generic `user.plo`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the first minimal userspace console sequence to the generic image so `pl011-tty` can create `/dev/console`

## Scope

In scope:

- update `_targets/aarch64a53/generic/user.plo.yaml`
- load `dummyfs` before `pl011-tty`
- rerun the generic QEMU smoke lane

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
- comparable QEMU `user.plo` files
- `phoenix-rtos-devices/tty/pl011-tty/*`
- `docs/status.md`
- tracking files and manifest updates for this step
- generic QEMU smoke output

## Acceptance Criteria

- the generic `user.plo` now loads `dummyfs` before `pl011-tty`
- the generic QEMU smoke lane is rerun from refreshed artifacts
- the change stays smaller than adding `psh`

## Validation Plan

- Review:
  inspect the `user.plo` edit against comparable QEMU script patterns and keep it minimal
- Build:
  rebuild the generic project/image artifacts needed for the smoke lane in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-user-plo-scope.md`

## Notes

- Risks:
  the result must stay as one `user.plo` integration step and must not silently turn into full shell bring-up
- Dependencies:
  completed implementation step `STEP-0086`
- User-visible control point before next step:
  after this runtime-image step lands, the next step should be chosen from the new smoke output
