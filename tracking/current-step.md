# Current Step

## Metadata

- Step ID: `STEP-0393`
- Title: Scope the smallest refreshed Pi 4 SD-image export step for the first HDMI plus USB-keyboard trial
- Status: `in_progress`
- Date: `2026-03-22`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest artifact-handoff step after the Pi 4 live USB-host image
  integration is complete

## Scope

In scope:

- defining the next smallest handoff artifact for real-device Pi 4 testing
- keeping the step focused on image export and operator readiness
- deciding whether any further code-side blocker still exists before exporting
  the SD-card image

Out of scope:

- new xHCI feature work
- new keyboard-path source changes
- manual hardware execution itself

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `tracking/current-step.md`
- `tracking/step-history.md`
- `docs/source-artifacts.md`
- `manifests/`

## Acceptance Criteria

- the smallest next artifact step is explicitly identified
- the docs reflect that `/sbin/usb` is already live on the Pi 4 image path
- the next implementation step is clearly bounded as either SD-image export or
  a specific newly discovered blocker

## Validation Plan

- inspect the current integrated image state and the existing validation
  evidence
- confirm whether any remaining blocker is code-side or only hardware-side

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-22-rpi4b-usb-staging.md`

## Notes

- Risks:
  avoid widening the step into extra xHCI work if the current board image is
  already ready for handoff
- Dependencies:
  completed `STEP-0392` live USB-host integration step
- User-visible control point before next step:
  after this step, the next bounded move should be a refreshed SD-image export
  unless a concrete remaining blocker is identified
