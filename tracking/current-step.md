# Current Step

## Metadata

- Step ID: `STEP-0090`
- Title: Define the first generic userspace-start diagnostic step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- identify the smallest diagnostic step that can prove whether generic userspace startup is reaching the packaged console path

## Scope

In scope:

- inspect the updated smoke result after packaging `dummyfs`, `pl011-tty`, and `psh`
- choose the smallest runtime diagnostic that can distinguish “userspace not reached” from “userspace reached but silent”
- stop before implementing that diagnostic

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
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

- the next diagnostic step is selected from the updated smoke evidence
- the follow-up stays as one small implementation commit where possible
- the selected step advances the generic QEMU fast lane directly

## Validation Plan

- Review:
  inspect the updated runtime state and keep the selected diagnostic minimal
- Build:
  use existing runtime evidence and nearby code patterns to choose the smallest useful diagnostic
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-psh.md`

## Notes

- Risks:
  the result must stay as a diagnostic-planning step and must not silently turn into multi-change bring-up
- Dependencies:
  completed implementation step `STEP-0089`
- User-visible control point before next step:
  after the diagnostic step is selected, the follow-up implementation should stay narrow and validation-driven
