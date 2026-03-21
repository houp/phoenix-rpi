# Current Step

## Metadata

- Step ID: `STEP-0280`
- Title: Scope the first macOS flashing-workflow documentation step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- select the smallest next documentation step that lets the operator safely put
  the current Pi 4 SD-card image onto real media

## Scope

In scope:

- decide the smallest operator-facing flashing guidance to add now that the
  host-visible `rpi4b-sd.img` exists
- keep the step no-hardware and documentation-focused
- keep the guidance aligned with macOS plus the current no-UART hardware lab

Out of scope:

- implementing the flashing instructions themselves
- changing Phoenix source code
- real hardware execution

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- host-visible `artifacts/rpi4b/rpi4b-sd.img`
- current manual operator runbook
- current no-UART lab note
- `docs/status.md`
- `docs/manual-operator-instructions.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the next flashing-guidance step is selected explicitly
- the selected move stays documentation-only and does not widen into hardware
  execution or automation
- no Phoenix upstream repo changes are introduced

## Validation Plan

- inspect the current host-visible `rpi4b-sd.img`
- confirm the next missing operator link is explicit flashing guidance rather
  than another artifact transformation
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-sdimg-export-helper.md`

## Notes

- Risks:
  avoid widening into SD-writing automation, real-device execution, or
  framebuffer or network bring-up
- Dependencies:
  completed `STEP-0279` SD-card image export helper
- User-visible control point before next step:
  after the scope decision, the next bounded step can add one concrete macOS
  flashing workflow to the runbook
