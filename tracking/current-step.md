# Current Step

## Metadata

- Step ID: `STEP-0059`
- Title: Run first end-to-end `aarch64a53-generic-qemu` smoke command
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- execute the first bounded end-to-end runtime smoke command for the new `aarch64a53-generic-qemu` entry point on QEMU `virt`

## Scope

In scope:

- run the selected smoke command in `phoenix-dev`
- capture the first serial behavior from the current launcher unchanged
- stop after recording whether the current generic QEMU lane reaches the loader banner or which earliest runtime failure occurs

Out of scope:

- implementation code in upstream Phoenix repositories
- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- fixing the runtime failure in this step unless the smoke unexpectedly passes and only documentation closure is needed

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

- the selected smoke command is executed in `phoenix-dev`
- the result records whether the loader banner appears on serial output
- if the loader banner does not appear, the result records the earliest observed failure mode without widening into a fix step

## Validation Plan

- Review:
  inspect the current launch script, artifact names, and generic `plo`/kernel constraints as needed during result analysis
- Build:
  reuse the currently prepared generic QEMU artifacts if needed
- Emulator:
  run `timeout 10s ./scripts/aarch64a53-generic-qemu.sh` inside the copied buildroot in `phoenix-dev`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-smoke-step-scope.md`

## Notes

- Risks:
  the result must stay as one smoke execution and must not silently turn into runtime debugging or cross-repo build unblock work
- Dependencies:
  completed implementation step `STEP-0058`
- User-visible control point before next step:
  after this smoke run lands, the next slice should be the smallest runtime-fix or stabilization step implied by the earliest observed result
