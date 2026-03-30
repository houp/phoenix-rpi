# Current Step

## Metadata

- Step ID: `STEP-0409`
- Title: Return to the Pi 4 hardware boundary after the upstream sync
- Status: `pending`
- Date: `2026-03-30`
- Milestone / phase: `Phase 1`

## Objective

- hold the project at the correct post-sync boundary:
  the refreshed Pi 4 SD image is ready and the next stronger lane is the first
  real hardware run

## Scope

In scope:

- verifying that the upstream sync closeout is fully recorded
- keeping the refreshed Pi 4 artifact and runbook as the current handoff state
- waiting for the first real board result before widening into new runtime work

Out of scope:

- speculative pre-hardware runtime changes
- new USB or xHCI feature work without board evidence
- unrelated refactors

## Expected Repositories

- no code changes expected unless a documentation correction is needed

## Expected Files Or Subsystems

- `tracking/current-step.md`
- `docs/pi4-first-hardware-trial.md`
- `docs/manual-operator-instructions.md`
- `docs/status.md`

## Acceptance Criteria

- the project state clearly reflects that the upstream sync and retest are done
- the refreshed Pi 4 image and checksum remain the current handoff artifact
- the next stronger validation lane is explicitly the first real Pi 4 boot

## Validation Plan

- re-read the sync closeout manifest and current hardware runbook
- confirm the tracked repos remain clean

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-30-upstream-sync-and-retest.md`

## Notes

- Risks:
  avoid inventing new pre-hardware work without real board evidence
- Dependencies:
  `STEP-0408` completed and recorded
- User-visible control point before next step:
  the next bounded move should be the first real Pi 4 board run using the
  refreshed image
