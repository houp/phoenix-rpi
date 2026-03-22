# Current Step

## Metadata

- Step ID: `STEP-0394`
- Title: Implement the refreshed Pi 4 SD-image export step
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- rebuild and export the first Pi 4 SD-card image that includes the current
  HDMI text-console path plus the live staged USB-host path

## Scope

In scope:

- fresh Pi 4 A72 rebuild on the standard copied-buildroot lane
- bootfs and SD-card image regeneration
- SD-card image export into `artifacts/rpi4b/`
- documentation of the specific handoff artifact and first operator trial

Out of scope:

- new code-side USB or xHCI feature work
- manual hardware execution itself
- wider packaging such as `phoenix-rtos-ports`

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `scripts/assemble-rpi4b-bootfs.sh`
- `scripts/assemble-rpi4b-bootfs-img.sh`
- `scripts/assemble-rpi4b-sdimg.sh`
- `scripts/export-rpi4b-sdimg.sh`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- a refreshed Pi 4 SD image exists under `artifacts/rpi4b/`
- the exported image corresponds to the current staged `pcie -> usb -> psh`
  path
- the operator runbook points at that specific image for the first HDMI plus
  USB-keyboard trial

## Validation Plan

- fresh Pi 4 A72 build in `phoenix-dev`
- regenerate the Pi 4 bootfs and SD-card image
- export the SD-card image to the host workspace
- record the exported artifact path and checksum

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-sdimg-refresh-scope.md`

## Notes

- Risks:
  avoid widening the step into extra code work if the current integrated image
  can already be handed off for board validation
- Dependencies:
  completed `STEP-0393` SD-image refresh scope
- User-visible control point before next step:
  after this step, the next bounded move should be real-device execution or a
  specific hardware-discovered blocker
