# Current Step

## Metadata

- Step ID: `STEP-0062`
- Title: Define first boot-output debugging step for silent generic QEMU lane
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- identify the smallest next change that can turn the silent generic QEMU smoke lane into visible early boot output

## Scope

In scope:

- inspect the silent post-launch behavior from `STEP-0061`
- compare the generic launcher and console assumptions with the current generic `plo` scaffold
- choose the narrowest first boot-output debugging step

Out of scope:

- broad generic QEMU bring-up changes
- `phoenix-rtos-tests` target additions
- Raspberry Pi-specific code
- implementing the fix in this planning step

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic-qemu/`
- `phoenix-rtos-project/_projects/aarch64a53-generic-qemu/`
- `phoenix-rtos-project/scripts/aarch64a53-generic-qemu.sh`
- `plo/hal/aarch64/generic/console.c`
- `plo/hal/aarch64/generic/config.h`
- `docs/status.md`
- tracking files and manifest updates for this step
- smoke output captured from the copied buildroot in `phoenix-dev`

## Acceptance Criteria

- the result names the smallest concrete next fix or experiment to recover first boot output
- the result explains why that step is preferred over broader entry-path or kernel changes
- the step remains planning-only

## Validation Plan

- Review:
  inspect the generic launcher, generic `plo` console assumptions, and comparable QEMU serial-routing patterns
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-launcher-fix.md`

## Notes

- Risks:
  the result must stay as one planning step for first boot-output recovery and must not silently turn into broader QEMU or kernel bring-up
- Dependencies:
  completed implementation step `STEP-0061`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected first boot-output recovery change
