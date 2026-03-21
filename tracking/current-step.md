# Current Step

## Metadata

- Step ID: `STEP-0300`
- Title: Scope the first manual Pi 4 board trial with staged HDMI progress
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the exact first manual Pi 4 board trial using the refreshed SD image
  and current no-UART HDMI expectations

## Scope

In scope:

- select the smallest manual execution plan for the first real board attempt
- keep the scope to operator actions and expected observations

Out of scope:

- flashing or hardware execution
- new firmware or runtime logic
- new artifact-export work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `manifests/`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the first manual board trial is reduced to a concrete short sequence
- the expected HDMI observations are explicit
- the step stays manual-only and does not quietly widen back into code changes

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-staged-hdmi-image-refresh.md`

## Notes

- Risks:
  avoid broadening a manual trial scope step into new implementation work
- Dependencies:
  completed `STEP-0299` refreshed Pi 4 SD-image export
- User-visible control point before next step:
  after this step lands, the project should be fully ready for the user to
  flash the card and perform the first manual Pi 4 board trial
