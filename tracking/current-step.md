# Current Step

## Metadata

- Step ID: `STEP-0257`
- Title: Scope the smallest interactive shell-smoke validation step
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest command-level QEMU validation that proves the new
  `(psh)%` prompt is genuinely interactive on both generic and Pi 4 fast lanes

## Scope

In scope:

- inspect the available `psh` command options for the smallest deterministic
  smoke command
- define how to drive one command through QEMU on:
  - `aarch64a53-generic-qemu`
  - `aarch64a72-generic-rpi4b`
- keep the step limited to validation method selection and acceptance markers

Out of scope:

- changing shell code or image contents
- broader shell test automation
- boot-media work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-utils/psh/`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- one concrete smoke command is selected
- its expected success markers are defined for both QEMU fast lanes
- the execution method is bounded enough to implement in the next step without
  widening into a full test harness

## Validation Plan

- Source review:
  inspect the relevant `psh` command path and output expectations
- Runtime planning:
  reuse the existing prompt-reaching QEMU launch method and define the minimum
  stdin-driving approach for the next step
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-console-probe-cleanup.md`

## Notes

- Risks:
  avoid turning this into a broad shell test or harness-design step
- Dependencies:
  completed `STEP-0256` prompt-preserving probe cleanup
- Source reminder:
  keep the next step focused on one command path, not the whole shell
- User-visible control point before next step:
  after this scope is fixed, the next step should execute exactly that one
  command on both QEMU lanes
