# Current Step

## Metadata

- Step ID: `STEP-0060`
- Title: Define smallest generic QEMU launcher-fix step
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest code change needed to let the generic QEMU smoke lane start QEMU after the first smoke run failed at launcher invocation

## Scope

In scope:

- inspect the launcher-level failure from `STEP-0059`
- choose the narrowest fix that allows the smoke lane to start QEMU
- keep the step planning-only and stop before changing code or rerunning QEMU

Out of scope:

- implementation code in upstream Phoenix repositories
- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- QEMU reruns or boot-path debugging

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic-qemu/`
- `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/`
- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- `docs/status.md`
- tracking files and manifest updates for this step
- smoke output captured from the copied buildroot in `phoenix-dev`

## Acceptance Criteria

- the result names the smallest concrete launcher fix to apply next
- the result explains why that fix is preferred over broader QEMU or boot-path changes
- the step remains planning-only

## Validation Plan

- Review:
  inspect the launcher mode and comparable existing QEMU launch scripts
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-first-smoke.md`

## Notes

- Risks:
  the result must stay as one launcher-fix planning step and must not silently turn into runtime debugging or broader build unblock work
- Dependencies:
  completed implementation step `STEP-0059`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected one-file launcher fix
