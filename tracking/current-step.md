# Current Step

## Metadata

- Step ID: `STEP-0108`
- Title: Define the first Pi 4 DTB propagation step into the kernel-visible payload
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- bound the smallest project-local step that gets a real Pi 4 DTB into the kernel-visible payload path as `system.dtb`

## Scope

In scope:

- inspect the current Pi 4 `user.plo.yaml` and the generic kernel DTB requirement
- confirm that the current Pi 4 payload does not provide `system.dtb`
- define the smallest project-local way to propagate a supplied Pi 4 DTB into the kernel-visible payload
- keep the next step bounded to project-local build and script glue if possible

Out of scope:

- broad Pi 4 storage-driver work
- generic loader entry refactors beyond the now-validated multi-EL patch
- kernel Pi 4 driver enablement
- new kernel DTB parser work
- firmware policy changes unrelated to getting `system.dtb` into the existing payload path
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration

## Expected Repositories

- coordination repo

## Expected Files Or Subsystems

- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/user.plo.yaml`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/build.project`
- `phoenix-rtos-project/_projects/aarch64a53-generic-rpi4b/config.txt`
- `phoenix-rtos-kernel/hal/aarch64/hal.c`
- documentation and manifest updates for this planning step

## Acceptance Criteria

- the current Pi 4 DTB gap is explicitly documented from source inspection and existing build artifacts
- the next implementation step is fixed as one bounded project-local DTB propagation change
- the selected approach keeps the generic kernel contract of loading `system.dtb` rather than inventing a Pi 4-only kernel handoff

## Validation Plan

- Review:
  inspect the current DTB flow from firmware tree to payload to kernel entry
- Build:
  not applicable
- Emulator:
  define whether the selected DTB propagation change has any no-hardware validation lane beyond build artifact inspection
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-rpi4b-payload-staging.md`

## Notes

- Risks:
  the next step must not silently widen into storage drivers or firmware-to-loader DTB parsing unless a smaller payload-side reuse path is ruled out first
- Dependencies:
  completed implementation step `STEP-0107`
- User-visible control point before next step:
  once the DTB propagation step is defined, the next implementation patch should be a single project-local change that moves the Pi 4 image closer to a first real board boot
