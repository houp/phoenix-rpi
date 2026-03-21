# Current Step

## Metadata

- Step ID: `STEP-0268`
- Title: Implement the distinct-output external-applet smoke
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- validate one external-applet command whose output is distinct from its echoed
  invocation text

## Scope

In scope:

- run:
  - `echo -h`
- verify:
  - command echo after the prompt
  - `Usage: echo [options] [string]`
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

- both fast lanes pass the `echo -h` smoke
- the usage string is visible in the command output band
- no Phoenix source changes are introduced

## Validation Plan

- Emulator:
  reuse the current `expect`-driven pattern with `echo -h`
- Matching:
  capture the command echo, usage string, and returned prompt on both lanes
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-distinct-output-applet-scope.md`

## Notes

- Risks:
  avoid widening into broad shell test design
- Dependencies:
  completed `STEP-0267` distinct-output applet scoping
- Source reminder:
  keep the output marker distinct from the echoed command
- User-visible control point before next step:
  after this validation lands, the next step can decide whether to fold the new
  command into the helper or pivot back toward boot-first system work
