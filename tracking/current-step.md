# Current Step

## Metadata

- Step ID: `STEP-0277`
- Title: Implement the smallest full Pi 4 SD-card image helper
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest helper that turns the current Pi 4 boot payload into a
  normal flashable SD-card image

## Scope

In scope:

- add one helper that wraps the current `rpi4b-bootfs.img` in a one-partition
  MBR disk image
- validate the partition table and the embedded FAT payload
- keep the step no-hardware and operator-facing

Out of scope:

- adding persistent-rootfs partitions
- changing Phoenix source code
- real hardware execution

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `scripts/assemble-rpi4b-bootfs-img.sh`
- exported `artifacts/rpi4b/rpi4b-bootfs.img`
- VM-local `rpi4b-bootfs.img`
- new full-image helper
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the helper produces one full Pi 4 disk image suitable for normal SD-card
  flashing workflows
- the resulting image has a valid MBR plus one bootable FAT partition
- the embedded FAT partition still contains the expected firmware-visible boot
  file set
- no Phoenix upstream repo changes are introduced

## Validation Plan

- Helper validation:
  run the new full-image helper
- Layout validation:
  inspect the resulting partition table inside `phoenix-dev`
- Payload validation:
  inspect the embedded FAT partition contents from the resulting disk image
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-sdimg-scope.md`

## Notes

- Risks:
  avoid widening into persistent-storage or multi-partition runtime work
- Dependencies:
  completed `STEP-0276` SD-card image scoping
- User-visible control point before next step:
  after this helper lands, the next bounded move can export that SD-card image
  to the host and document the first actual flashing workflow
