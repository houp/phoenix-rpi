# Current Step

## Metadata

- Step ID: `STEP-0266`
- Title: Implement the unique-token external-applet smoke
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- validate one deterministic external-applet command with a unique output token
  on both fast lanes

## Scope

In scope:

- run:
  - `echo codex-smoke-echo`
- verify:
  - command echo after the prompt
  - `codex-smoke-echo`
  - returned prompt
- keep the step runtime-only

Out of scope:

- changing Phoenix source code
- extending the smoke set beyond this one command
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

- both fast lanes pass the unique-token applet smoke
- the token `codex-smoke-echo` is visible in the command output band
- no Phoenix source changes are introduced

## Validation Plan

- Emulator:
  reuse the current `expect`-driven pattern with `echo codex-smoke-echo`
- Matching:
  capture the command echo, token output, and returned prompt on both lanes
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-unique-token-applet-scope.md`

## Notes

- Risks:
  avoid widening into broad shell test design
- Dependencies:
  completed `STEP-0265` unique-token applet scoping
- Source reminder:
  keep the token unique enough to avoid accidental matches
- User-visible control point before next step:
  after this validation lands, the next step can decide whether to fold the new
  command into the helper or pivot back toward boot-first system work
