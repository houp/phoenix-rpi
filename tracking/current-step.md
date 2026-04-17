# Current Step

## Metadata

- Step ID: `STEP-0487`
- Title: Re-establish a reproducible committed Pi 4 userspace-boot baseline
- Status: `ready`
- Date: `2026-04-17`
- Milestone / phase: `Phase 1`

## Objective

- rebuild the Pi 4 image from a fully committed and reproducible source state
- decide whether the remaining `plo` ACT-LED telemetry cleanup should be
  committed or explicitly dropped before the next hardware retry
- align the exported image, tracker, and manifests to the exact committed repo
  SHAs instead of relying on a mixed committed-plus-dirty local state

## Scope

In scope:
- decide the fate of the dirty `plo/hal/aarch64/generic/_init.S` cleanup diff
- rebuild, export, and verify a Pi 4 SD image from a fully committed source set
- record the exact repo SHAs and resulting image SHA-256 in a manifest
- keep `docs/status.md`, `tracking/current-step.md`, and
  `tracking/step-history.md` aligned with the actual committed tree

Out of scope:
- new Pi 4 boot logic changes beyond resolving the reproducibility gap
- new hardware-trial interpretation before the reproducible image exists

## Acceptance Criteria

- all touched upstream repos are either clean or have their remaining dirt
  explicitly documented as intentional and excluded from the next image
- a fresh exported Pi 4 SD image is produced from that committed state
- the image sidecar, manifest, and tracker all agree on the same SHA-256
- the tracker no longer claims that legacy `plo` / `dummyfs` LED probes are
  purged unless that is true in the committed source tree

## Validation Plan

- inspect `git status --short` across the coordination repo and touched sibling
  repos
- rebuild with `./scripts/rebuild-rpi4b-fast.sh`
- export with `./scripts/export-rpi4b-sdimg.sh`
- verify with `./scripts/verify-rpi4b-sdimg.sh`

## Rollback / Baseline

- last exported Pi 4 image currently present on the host:
  `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  (SHA-256: `2f2be2e7bc97500e5202ee55f960a9b1423a79d611112d527fd35868bdec5527`)
- current committed repo heads recorded in
  `manifests/2026-04-17-pi4-tracker-reconciliation.md`

## Notes

- the coordination repo had drifted ahead of the actual committed source state:
  `docs/status.md` was stale, `tracking/current-step.md` overstated LED-probe
  cleanup, and recent April 14-16 source steps were not recorded in a manifest
- the committed tree still contains active Pi 4 LED diagnostics today:
  - `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`
    still defines `PLO_RPI_ACTLED_DIAG 1`
  - `sources/phoenix-rtos-filesystems/dummyfs/srv.c` still emits the Stage-5
    GPIO42 signal after `devfs` registration
  - `sources/plo/hal/aarch64/generic/_init.S` cleanup exists only as an
    uncommitted local diff and is not part of the last reproducible baseline
