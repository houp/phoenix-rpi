# Current Step

## Metadata

- Step ID: `STEP-0262`
- Title: Implement the reusable QEMU shell-smoke helper
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest reusable helper for the validated QEMU `help` shell
  smoke

## Scope

In scope:

- add `scripts/qemu-shell-smoke.sh`
- support only:
  - `generic`
  - `rpi4b`
- keep the smoke command fixed to `help`
- rerun the helper for both targets

Out of scope:

- changing Phoenix source code
- broad QEMU test-harness design
- boot-media work
- real hardware work

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- QEMU runtime validation flow in `phoenix-dev`
- existing generic and Pi 4 smoke logs in `/tmp`
- `scripts/`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the helper runs successfully for both supported targets
- it proves the same `help` smoke markers already validated by hand
- no Phoenix upstream repo changes are needed

## Validation Plan

- Script validation:
  run `scripts/qemu-shell-smoke.sh generic`
  run `scripts/qemu-shell-smoke.sh rpi4b`
- Matching:
  confirm each run reaches `Available commands:` and the returned prompt
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-qemu-shell-smoke-helper-scope.md`

## Notes

- Risks:
  avoid turning a small smoke helper into a full lab framework
- Dependencies:
  completed `STEP-0261` helper scoping
- Source reminder:
  both fast lanes now pass the same `help` smoke
- User-visible control point before next step:
  after this helper lands, the next step can extend smoke depth or shift back
  toward boot-first system work
