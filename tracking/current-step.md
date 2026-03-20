# Current Step

## Metadata

- Step ID: `STEP-0078`
- Title: Define the first generic AArch64 devices-target step for the PL011 console path
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- identify the smallest repo-local `phoenix-rtos-devices` step that prepares the first reusable PL011 console path for generic QEMU and Raspberry Pi 4

## Scope

In scope:

- inspect the current `phoenix-rtos-devices` target layout and nearby tty-driver patterns
- identify the smallest generic AArch64 devices-target change needed before a PL011 driver can be integrated
- stop before changing driver source code

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `docs/status.md`
- tracking files and manifest updates for this step
- `phoenix-rtos-devices/_targets/*`
- direct generic devices-target build output if needed for scoping

## Acceptance Criteria

- the smallest generic devices-target step is selected from actual repo structure
- the step keeps scope inside `phoenix-rtos-devices` where possible
- the follow-up implementation step is narrow enough to land as one repo-local commit

## Validation Plan

- Review:
  inspect `phoenix-rtos-devices` target files and nearby tty-driver patterns
- Build:
  use direct `phoenix-rtos-devices` target validation only if it helps confirm the smallest missing target-layer piece
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-runtime-unblock-scope.md`

## Notes

- Risks:
  the result must stay as a `phoenix-rtos-devices` discovery-and-scoping step and must not silently turn into multi-repo implementation work
- Dependencies:
  completed implementation step `STEP-0077`
- User-visible control point before next step:
  after the devices-target step is selected, the follow-up implementation step should stay repo-local and validation-driven
