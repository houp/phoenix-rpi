# Current Step

## Metadata

- Step ID: `STEP-0080`
- Title: Define the first reusable PL011 tty driver step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- identify the smallest reusable PL011 tty-driver slice that can advance generic QEMU and Raspberry Pi 4 bring-up without widening into a full driver implementation

## Scope

In scope:

- inspect nearby tty driver patterns in `phoenix-rtos-devices`
- inspect current generic-QEMU and Pi 4 console expectations from the coordination docs and build scripts
- choose the smallest repo-local PL011 driver slice

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
- `docs/status.md`
- tracking files and manifest updates for this step
- direct code references and, if needed, direct generic-target validation output

## Acceptance Criteria

- the first PL011 driver slice is selected from actual nearby driver patterns
- the selected slice stays inside `phoenix-rtos-devices` where possible
- the follow-up implementation step is narrow enough to land as one repo-local commit

## Validation Plan

- Review:
  inspect nearby tty drivers and keep the selected PL011 slice minimal
- Build:
  use direct repo validation only if it helps confirm the smallest missing PL011-layer piece
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-devices-target.md`

## Notes

- Risks:
  the result must stay as a PL011-driver planning step and must not silently turn into a broad multi-driver or multi-repo implementation
- Dependencies:
  completed implementation step `STEP-0079`
- User-visible control point before next step:
  after the first PL011 slice is selected, the follow-up implementation step should stay repo-local and validation-driven
