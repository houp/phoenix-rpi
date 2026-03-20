# Current Step

## Metadata

- Step ID: `STEP-0109`
- Title: Propagate the Pi 4 DTB into the kernel-visible payload
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest project-local step that gets a supplied Pi 4 DTB into the kernel-visible payload path as `system.dtb`

## Scope

In scope:

- update `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- update `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
- when a Pi 4 DTB is supplied, place it in `${PREFIX_ROOTFS}/etc/system.dtb`
- restore the matching `blob {{ env.BOOT_DEVICE }} /etc/system.dtb ddr` load entry

Out of scope:

- broad Pi 4 storage-driver work
- generic loader entry refactors beyond the now-validated multi-EL patch
- kernel Pi 4 driver enablement
- new kernel DTB parser work
- firmware policy changes unrelated to getting `system.dtb` into the existing payload path
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- `_fs/aarch64a53-generic-rpi4b/root/etc/system.dtb`
- generated Pi 4 `user.plo` script
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- when a Pi 4 DTB is supplied, the build places it at `${PREFIX_ROOTFS}/etc/system.dtb`
- the Pi 4 payload now loads `system.dtb` before the kernel handoff
- default no-DTB builds remain green if the propagation is implemented conditionally

## Validation Plan

- Review:
  inspect the DTB propagation path for minimality and consistency with the existing Pi 4 project-local build glue
- Build:
  run the Pi 4 project build
- Emulator:
  not required
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-dtb-propagation-scope.md`

## Notes

- Risks:
  the step must stay project-local and must not silently widen into runtime DTB parsing or bootloader protocol changes
- Dependencies:
  completed planning step `STEP-0108`
- User-visible control point before next step:
  after this step lands, the next bounded decision should come from the resulting Pi 4 image state, likely around firmware DTB compatibility or the first real board smoke attempt
