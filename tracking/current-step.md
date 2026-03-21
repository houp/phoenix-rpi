# Current Step

## Metadata

- Step ID: `STEP-0286`
- Title: Scope the smallest real-hardware-oriented HDMI refinement for the first Pi 4 trial
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next move that improves the odds and interpretability of
  the first real Pi 4 HDMI trial without widening into a full display stack

## Scope

In scope:

- review the current firmware-facing Pi 4 HDMI staging assumptions
- identify one minimal refinement for the first real board trial
- prefer firmware config or operator-facing expectations over new graphics logic

Out of scope:

- runtime framebuffer console support
- keyboard or mouse UI support
- broad graphics subsystem work
- real hardware execution in this step

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `docs/manual-operator-instructions.md`
- `docs/status.md`
- `docs/source-artifacts.md`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/config.txt`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- one explicit next refinement is selected for the first real Pi 4 HDMI trial
- the refinement stays small and does not widen into runtime graphics support
- the result records what should be changed or validated next and why

## Validation Plan

- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-rpi4b-hdmi-smoke-helper.md`

## Notes

- Risks:
  avoid turning a firmware-config refinement into a broad display-policy change
- Dependencies:
  completed `STEP-0285` Pi 4 HDMI smoke helper
- User-visible control point before next step:
  after this step lands, the next bounded move should either implement the
  selected firmware or operator refinement or return to the next common Pi 4
  runtime blocker
