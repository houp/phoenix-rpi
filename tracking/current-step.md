# Current Step

## Metadata

- Step ID: `STEP-0106`
- Title: Define the first Pi 4 firmware payload-staging step after multi-EL loader bring-up
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- bound the smallest useful Pi 4 boot-tree change that supplies the existing generic AArch64 `plo` path with its payload after Raspberry Pi firmware boots `kernel8.img`

## Scope

In scope:

- inspect the current Pi 4 project-local boot tree and the generic AArch64 preinit assumptions
- confirm how the current generic `ram0` / `loader.disk` path works
- determine the smallest Pi 4-compatible way to preload that payload without adding an SD or FAT driver
- define the next implementation step and validation method

Out of scope:

- broad Pi 4 storage-driver work
- generic loader entry refactors beyond the now-validated multi-EL patch
- kernel Pi 4 driver enablement
- DTB staging policy changes
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/config.txt`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
- `phoenix-rtos-project/_targets/aarch64a53/generic/build.project`
- `phoenix-rtos-project/_targets/aarch64a53/generic/preinit.plo.yaml`
- documentation and manifest updates for this planning step

## Acceptance Criteria

- the current Pi 4 project-local payload gap is explicitly documented from source inspection
- the next implementation step is fixed as one bounded boot-tree or project-local staging change
- the selected next step reuses the existing generic `plo` payload path rather than inventing a new board-specific loader mechanism unless evidence forces it

## Validation Plan

- Review:
  inspect the current generic payload-loading path and the Pi 4 boot tree
- Build:
  not applicable
- Emulator:
  define whether an existing no-hardware lane can validate the selected payload-staging change
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-plo-entry.md`

## Notes

- Risks:
  the next step must not silently widen into SD, FAT, or network drivers unless a smaller payload-staging reuse path is ruled out first
- Dependencies:
  completed implementation step `STEP-0105`
- User-visible control point before next step:
  once the payload-staging approach is defined, the next implementation step should be a single project-local change that moves the Pi 4 boot tree closer to a real firmware boot without needing real hardware yet
