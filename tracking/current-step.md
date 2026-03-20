# Current Step

## Metadata

- Step ID: `STEP-0125`
- Title: Implement raw UART-side registration diagnostics in `pl011-tty`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest high-signal diagnostics that can show exactly where `pl011-tty` stops between startup and console registration

## Scope

In scope:

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- add raw UART-side progress markers around:
  - `/dev/tty0` registration attempt
  - `/dev/tty0` registration result
  - `/dev/console` registration attempt
  - `/dev/console` registration result
- keep the patch in shared userspace/device code
- validate on both the generic and Pi 4 DTB-backed QEMU lanes

Out of scope:

- broad Pi 4 peripheral-debug work
- new board-specific DTB work
- broad init-sequencing redesign across loader scripts and services
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- speculative retry or init-order work beyond the diagnostic markers themselves

## Expected Repositories

- `sources/phoenix-rtos-devices`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the next QEMU run produces at least one new raw registration marker after `pl011-tty: started`
- the markers distinguish whether the current boundary is before, during, or after `/dev/tty0` registration
- neither lane regresses from current known-good startup output

## Validation Plan

- Review:
  confirm that the patch stays local to `pl011-tty` and only adds raw diagnostics
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
  `manifests/2026-03-20-aarch64-pl011-registration-diagnostic-scope.md`

## Notes

- Risks:
  avoid turning diagnostics into a permanent large debug scaffold
- Dependencies:
  completed `STEP-0124`
- User-visible control point before next step:
  after this step lands, the next bounded move should come from concrete new console output on the two QEMU lanes
