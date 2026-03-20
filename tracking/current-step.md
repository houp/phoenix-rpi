# Current Step

## Metadata

- Step ID: `STEP-0123`
- Title: Implement bounded `create_dev()` retry handling in `pl011-tty`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest shared console-readiness patch that can move both the generic and Pi 4 DTB-backed QEMU lanes beyond `pl011-tty: started`

## Scope

In scope:

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- add a small bounded retry helper around `create_dev()`
- use it for `/dev/tty0` and `_PATH_CONSOLE`
- keep the patch in shared userspace/device code
- validate on both the generic and Pi 4 DTB-backed QEMU lanes

Out of scope:

- broad Pi 4 peripheral-debug work
- new board-specific DTB work
- broad init-sequencing redesign across loader scripts and services
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- unrelated kernel early-console work

## Expected Repositories

- `sources/phoenix-rtos-devices`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- `pl011-tty` no longer fails on the first transient registration miss
- at least one lane advances beyond the current `pl011-tty: started` boundary
- the other lane does not regress from the current known-good output

## Validation Plan

- Review:
  confirm that the patch stays local to `pl011-tty` and uses bounded retry behavior
- Build:
  rebuild the affected device and project lanes in `phoenix-dev`
- Emulator:
  rerun:
  - generic `virt`
  - Pi 4 DTB-backed `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-pl011-console-retry-scope.md`

## Notes

- Risks:
  keep retry timing bounded and driver-local
- Dependencies:
  completed `STEP-0122`
- User-visible control point before next step:
  after this step lands, the next bounded move should come from concrete new console output on the two QEMU lanes
