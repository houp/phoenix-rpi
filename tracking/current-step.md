# Current Step

## Metadata

- Step ID: `STEP-0299`
- Title: Refresh the host-visible Pi 4 SD image after staged HDMI progress
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- refresh the host-visible Pi 4 SD image so the real board uses the staged HDMI
  progress indicator instead of the older single-rectangle marker

## Scope

In scope:

- rerun the known-good full helper chain
- record the refreshed host-visible image checksum
- update the operator-facing docs with the new image identity

Out of scope:

- new firmware or runtime logic
- flashing or hardware execution
- changes to the established helper sequence

## Expected Repositories

- coordination repo
- helper scripts only as already established

## Expected Files Or Subsystems

- `artifacts/rpi4b/rpi4b-sd.img`
- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `manifests/`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the full helper chain completes successfully
- `artifacts/rpi4b/rpi4b-sd.img` is refreshed
- the new checksum is recorded in the docs and manifest

## Validation Plan

- Build:
  use the already rebuilt Pi 4 outputs as input to the helper chain
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-staged-hdmi-refresh-scope.md`

## Notes

- Risks:
  avoid assuming the export helper alone is sufficient; the full helper chain is
  still required
- Dependencies:
  completed `STEP-0298` refreshed-image handoff scope
- User-visible control point before next step:
  after this step lands, the current host-visible artifact should be ready for
  a manual Pi 4 board trial with the staged HDMI panel
