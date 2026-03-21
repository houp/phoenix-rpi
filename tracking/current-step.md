# Current Step

## Metadata

- Step ID: `STEP-0258`
- Title: Implement the first interactive shell-smoke validation
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- execute one deterministic shell command through the new `(psh)%` prompt on
  both generic and Pi 4 fast lanes

## Scope

In scope:

- launch interactive QEMU sessions for:
  - `aarch64a53-generic-qemu`
  - `aarch64a72-generic-rpi4b`
- wait for the `(psh)%` prompt
- send `help`
- verify:
  - `Available commands:`
  - returned `(psh)%`

Out of scope:

- changing source code
- broader shell test automation or harness design
- boot-media work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- QEMU runtime validation flow in `phoenix-dev`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- generic QEMU accepts `help` and returns to `(psh)%`
- Pi 4 QEMU accepts `help` and returns to `(psh)%`
- no source changes are needed for this validation step

## Validation Plan

- Emulator:
  interactive TTY QEMU sessions with command injection via stdin
- Matching:
  capture the first `help` output band and the returned prompt in both lanes
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-shell-smoke-scope.md`

## Notes

- Risks:
  avoid widening the step into persistent automation or shell feature work
- Dependencies:
  completed `STEP-0257` interactive smoke scoping
- Source reminder:
  keep the step limited to the built-in `help` command path
- User-visible control point before next step:
  after this validation lands, the next step can turn it into a reusable smoke
  loop or move to the next boot-first target
