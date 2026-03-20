# Current Step

## Metadata

- Step ID: `STEP-0088`
- Title: Define the first generic `psh` integration step after the console-prep smoke rerun
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- identify whether the next smallest runtime step is to add `psh` to the generic image or to add a narrower userspace-console diagnostic first

## Scope

In scope:

- inspect the updated generic smoke result
- inspect comparable minimal console-plus-shell user scripts
- choose the smallest useful follow-up step

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

- the next runtime step is selected from the updated smoke evidence
- the follow-up stays as one small implementation commit where possible
- the selected step advances the generic QEMU fast lane directly

## Validation Plan

- Review:
  inspect the updated smoke result against comparable minimal console and shell scripts
- Build:
  use direct build or runtime evidence only as needed to choose the smallest useful follow-up
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-user-plo.md`

## Notes

- Risks:
  the result must stay as a runtime-planning step and must not silently turn into multi-change generic bring-up
- Dependencies:
  completed implementation step `STEP-0087`
- User-visible control point before next step:
  after the next runtime step is selected, the follow-up implementation should stay narrow and validation-driven
