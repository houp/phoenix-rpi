# Current Step

## Metadata

- Step ID: `STEP-0263`
- Title: Scope the smallest external-applet shell smoke
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest next shell-level validation that goes beyond the built-in
  `help` command

## Scope

In scope:

- review the currently staged applets and namespace state
- choose one deterministic external-applet shell command for both fast lanes
- define the success markers for that command

Out of scope:

- changing Phoenix source code
- implementing the next shell smoke
- boot-media work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- QEMU runtime validation flow in `phoenix-dev`
- existing generic and Pi 4 smoke logs in `/tmp`
- `scripts/qemu-shell-smoke.sh`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- one deterministic external-applet command is selected
- its success markers are defined for both fast lanes
- the next step remains a small validation step

## Validation Plan

- Source review:
  inspect the available applets and current root namespace expectations
- Runtime planning:
  reuse the current shell-smoke helper pattern and select the next command
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-qemu-shell-smoke-helper.md`

## Notes

- Risks:
  avoid widening into broad shell test design
- Dependencies:
  completed `STEP-0262` helper implementation
- Source reminder:
  the next command should exercise more system surface than a built-in
- User-visible control point before next step:
  after this scope lands, the next step should run exactly one external-applet
  smoke on both fast lanes
