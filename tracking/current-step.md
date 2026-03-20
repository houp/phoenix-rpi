# Current Step

## Metadata

- Step ID: `STEP-0099`
- Title: Add the first Pi 4 project-local scaffold on top of `aarch64a53-generic`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest Pi 4-oriented scaffold that introduces board-local files without widening the existing target/subfamily matrix

## Scope

In scope:

- add a new project directory under `phoenix-rtos-project/_projects/`
- keep the existing `aarch64a53-generic` target and reuse its build flow
- add Pi 4-specific `board_config.h` and minimal `build.project`
- validate the new project through a no-hardware `host project image` build

Out of scope:

- new target families or subfamilies
- Pi 4 hardware drivers
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
- comparable QEMU `user.plo` files
- `phoenix-rtos-devices/tty/pl011-tty/*`
- `docs/status.md`
- tracking files and manifest updates for this step
- generic QEMU smoke output
- generic utils packaging expectations

## Acceptance Criteria

- `phoenix-rtos-project` exposes a new `aarch64a53-generic-rpi4b` project
- the new project keeps all target reuse within the existing generic subfamily
- the no-hardware `host project image` lane succeeds for the new project in `phoenix-dev`

## Validation Plan

- Review:
  inspect the project-local scaffold for minimality and target reuse
- Build:
  run `LIBPHOENIX_DEVEL_MODE=n TARGET=aarch64a53-generic-rpi4b ./phoenix-rtos-build/build.sh host project image` in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-project-scope.md`

## Notes

- Risks:
  the result must stay as one localized project-scaffold step and must not silently widen into kernel or loader board support
- Dependencies:
  completed planning step `STEP-0098`
- User-visible control point before next step:
  after the scaffold lands, the next Pi 4 follow-up should stay project-local or build-local unless a stronger reason emerges
