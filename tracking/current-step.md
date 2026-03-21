# Current Step

## Metadata

- Step ID: `STEP-0274`
- Title: Scope the smallest operator-facing Pi 4 FAT-image handoff step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next step that makes the current Pi 4 FAT image easier to
  hand off to the operator for real-device use

## Scope

In scope:

- review the current FAT image artifact and operator workflow
- choose one small improvement such as:
  - a checksum / metadata manifest
  - a host-side export helper
  - a clearer artifact naming / output convention
- keep the step no-hardware and operator-facing

Out of scope:

- changing Phoenix source code
- SD-card writing
- network boot setup
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- QEMU runtime validation flow in `phoenix-dev`
- existing generic and Pi 4 smoke logs in `/tmp`
- `scripts/qemu-shell-smoke.sh`
- Pi 4 firmware staging references
- Pi 4 `_boot/aarch64a72-generic-rpi4b/rpi4b/` outputs
- assembled `rpi4b-bootfs` tree
- FAT-image tools available in `phoenix-dev`
- assembled `rpi4b-bootfs.img`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- one concrete operator-facing improvement is selected
- it is justified by the current FAT-image-first device path
- the next step remains no-hardware and artifact-focused

## Validation Plan

- Workflow review:
  compare the current artifact outputs against the documented operator steps
- Documentation review:
  identify the smallest gap in operator-facing handling of the FAT image
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-post-fat-image-artifact-scope.md`

## Notes

- Risks:
  avoid widening into general deployment or storage work
- Dependencies:
  completed `STEP-0273` post-FAT-image artifact scoping
- Source reminder:
  the next step should leverage the current shell confidence, not revisit it
- User-visible control point before next step:
  after this scope lands, the next step should improve only one operator-facing
  aspect of the FAT image artifact
