# Current Step

## Metadata

- Step ID: `STEP-0264`
- Title: Implement the first external-applet shell smoke
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- validate one deterministic external-applet shell command on both fast lanes

## Scope

In scope:

- run:
  - `ls /`
- verify:
  - command echo after the prompt
  - `dev`
  - `syspage`
  - returned prompt
- keep the step limited to runtime validation and coordination updates

Out of scope:

- changing Phoenix source code
- extending the helper beyond this one new command
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

- both fast lanes pass the `ls /` smoke
- the output includes `dev` and `syspage`
- no Phoenix source changes are introduced

## Validation Plan

- Emulator:
  reuse the current `expect`-driven helper pattern for `ls /`
- Matching:
  capture the command echo, root entries, and returned prompt on both lanes
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-external-applet-smoke-scope.md`

## Notes

- Risks:
  avoid widening into broad shell test design
- Dependencies:
  completed `STEP-0263` external-applet smoke scoping
- Source reminder:
  keep the command surface minimal and deterministic
- User-visible control point before next step:
  after this validation lands, the next step can decide whether to extend the
  helper or shift back toward boot-first system work
