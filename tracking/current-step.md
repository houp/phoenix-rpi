# Current Step

## Metadata

- Step ID: `STEP-0406`
- Title: Implement the report-helper image-fingerprint refresh
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- make the first-trial report helper derive the current image fingerprint from
  the actual exported SD image

## Scope

In scope:

- deriving the report SHA-256 from the current image file
- keeping the helper non-destructive
- preserving the current operator-facing handoff flow

Out of scope:

- manual hardware execution itself
- new code-side USB or xHCI feature work until board evidence exists
- automatic flashing or other destructive helpers

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/pi4-first-hardware-trial.md`
- `scripts/create-rpi4b-first-trial-report.sh`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the report helper computes the current image fingerprint from the real file
- the generated report contains the current path and SHA-256
- no runtime behavior is changed

## Validation Plan

- run the report helper and inspect a newly generated report file

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-report-helper-refresh-scope.md`

## Notes

- Risks:
  avoid widening this into a larger reporting system
- Dependencies:
  completed `STEP-0405` report-helper refresh scope
- User-visible control point before next step:
  after this step, the next bounded move should again be the user's first board
  boot result or filled report file
