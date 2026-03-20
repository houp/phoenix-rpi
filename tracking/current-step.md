# Current Step

## Metadata

- Step ID: `STEP-0083`
- Title: Add `pl011-tty` to the generic devices target defaults
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- make the generic devices target build `pl011-tty` by default

## Scope

In scope:

- update `_targets/Makefile.aarch64a53-generic`
- keep the change repo-local to `phoenix-rtos-devices`
- validate `phoenix-rtos-devices all` on the generic target

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `phoenix-rtos-devices/_targets/*`
- `phoenix-rtos-devices/tty/*`
- `phoenix-rtos-devices/tty/pl011-tty/*`
- `docs/status.md`
- tracking files and manifest updates for this step
- direct generic devices build output

## Acceptance Criteria

- the generic devices target default components now include `pl011-tty`
- `phoenix-rtos-devices all` validates for `aarch64a53-generic-qemu`
- the change stays inside `phoenix-rtos-devices`

## Validation Plan

- Review:
  inspect the target-file change against nearby target-file style and keep it minimal
- Build:
  validate `phoenix-rtos-devices all` directly for `aarch64a53-generic-qemu` in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-target-integration-scope.md`

## Notes

- Risks:
  the result must stay as one repo-local target integration step and must not silently turn into board-config or `user.plo` wiring
- Dependencies:
  completed implementation step `STEP-0082`
- User-visible control point before next step:
  after the target integration lands, the next step should scope the first board-config wiring step
