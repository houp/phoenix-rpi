# Current Step

## Metadata

- Step ID: `STEP-0260`
- Title: Implement the generic-`virt` stdin-path adjustment
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- validate the smallest generic `virt` QEMU launch change for interactive shell
  command injection

## Scope

In scope:

- rerun the generic interactive shell smoke with:
  - `-nographic -monitor none`
- keep the built-in `help` smoke command unchanged
- verify whether the generic lane now matches the Pi 4 command-level result

Out of scope:

- changing Phoenix source code
- broader shell harness work
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

- generic `virt` either:
  - reaches `Available commands:` and a returned prompt with the adjusted
    launch, or
  - produces a tighter negative result tied specifically to the adjusted launch
- no Phoenix source changes are introduced

## Validation Plan

- Emulator:
  rerun the `expect`-driven generic smoke with the adjusted QEMU flags
- Evidence:
  save a fresh generic smoke log under `/tmp`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-generic-shell-stdin-scope.md`

## Notes

- Risks:
  avoid broad QEMU command rewrites when only stdin routing is under question
- Dependencies:
  completed `STEP-0259` generic stdin-path scoping
- Source reminder:
  Pi 4 already proves the shell command path itself
- User-visible control point before next step:
  after this validation lands, the next step can either adopt the new generic
  launch as the shell-smoke baseline or keep narrowing the generic runtime path
