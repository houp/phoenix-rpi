# Current Step

## Metadata

- Step ID: `STEP-0293`
- Title: Rerun the full Pi 4 artifact-refresh chain
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- refresh the host-visible Pi 4 SD image completely so the first board trial
  uses the latest HDMI firmware refinement

## Scope

In scope:

- rerun:
  - `scripts/assemble-rpi4b-bootfs.sh`
  - `scripts/assemble-rpi4b-bootfs-img.sh`
  - `scripts/assemble-rpi4b-sdimg.sh`
  - `scripts/export-rpi4b-sdimg.sh`
- record the refreshed host-visible checksum
- update the docs/status with the new artifact hash

Out of scope:

- flashing
- real hardware execution
- new firmware or runtime logic

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `scripts/assemble-rpi4b-bootfs.sh`
- `scripts/assemble-rpi4b-bootfs-img.sh`
- `scripts/assemble-rpi4b-sdimg.sh`
- `scripts/export-rpi4b-sdimg.sh`
- `artifacts/rpi4b/rpi4b-sd.img`
- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the full helper chain completes
- `artifacts/rpi4b/rpi4b-sd.img` is refreshed
- the new checksum is recorded
- the docs clearly indicate that the exported image now includes the HDMI
  firmware refinement

## Validation Plan

- Build:
  use the already rebuilt Pi 4 project outputs as the chain input
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-hdmi-firmware-refinement.md`

## Notes

- Risks:
  avoid mixing artifact refresh with flashing or board execution
- Dependencies:
  completed `STEP-0292` full artifact-refresh scoping
- User-visible control point before next step:
  after this step lands, the next bounded move can start the manual Pi 4 board
  trial or scope one more tiny pre-hardware refinement
