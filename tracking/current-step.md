# Current Step

## Metadata

- Step ID: `STEP-0259`
- Title: Scope the smallest generic-`virt` stdin-path adjustment
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest QEMU launch adjustment that should make the generic
  `virt` shell smoke accept automated stdin like the Pi 4 lane already does

## Scope

In scope:

- compare the current generic `virt` launch mode with the working Pi 4 launch
- identify the smallest generic runtime change to test next
- keep the change limited to the emulator invocation, not Phoenix source

Out of scope:

- changing Phoenix source code
- broad shell harness work
- boot-media work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- QEMU runtime validation flow in `phoenix-dev`
- existing generic and Pi 4 smoke logs in `/tmp`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- one minimal generic `virt` launch change is selected for the next step
- the rationale is tied to the observed generic-vs-Pi-4 difference
- the next step stays runtime-only

## Validation Plan

- Runtime review:
  compare the working Pi 4 smoke command with the failing generic command
- Evidence:
  use the saved shell-smoke logs and current QEMU arguments
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-shell-smoke-validation.md`

## Notes

- Risks:
  avoid broad QEMU command rewrites when only stdin routing is under question
- Dependencies:
  completed `STEP-0258` first shell-smoke validation
- Source reminder:
  Pi 4 already proves the shell command path itself
- User-visible control point before next step:
  after this scope lands, the next step should test exactly one adjusted
  generic QEMU launch path
