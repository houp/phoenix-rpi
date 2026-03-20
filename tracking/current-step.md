# Current Step

## Metadata

- Step ID: `STEP-0129`
- Title: Implement bounded post-lookup `create_dev()` diagnostics visible on stdout
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add the smallest user-space diagnostics that can show whether `create_dev("/dev/tty0")` blocks between the successful `lookup("devfs", ...)` return and the final `msgSend()` call

## Scope

In scope:

- `sources/libphoenix/unistd/file.c`
- add minimal diagnostics after the successful `lookup("devfs", ...)` return
- prefer a visibility path that does not depend on the user-space `debug()` syscall path
- distinguish:
  - post-lookup return
  - post-path-splitting progress
  - final `msgSend()` entry / return
- keep the patch reviewable, bounded, and temporary-diagnostic in nature
- validate on both the generic and Pi 4 DTB-backed QEMU lanes

Out of scope:

- broad Pi 4 peripheral-debug work
- new board-specific DTB work
- broad init-sequencing redesign across loader scripts and services
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- speculative functional fixes beyond the diagnostics themselves
- broad refactoring of `create_dev()` semantics

## Expected Repositories

- `sources/libphoenix`
- coordination repo

## Expected Files Or Subsystems

- `sources/libphoenix/unistd/file.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the next QEMU run produces at least one new post-lookup marker after the already-known `create_dev: lookup done` generic marker
- the markers distinguish whether the current boundary is in user-space path preparation or at final `msgSend()` entry / return
- neither lane regresses from current known-good startup output

## Validation Plan

- Review:
  confirm that the patch stays local to `create_dev()` and only adds narrow diagnostics
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
  `manifests/2026-03-20-aarch64-create-dev-syscall-diagnostics.md`

## Notes

- Risks:
  avoid turning shared libc code into a large permanent debug scaffold
- Dependencies:
  completed `STEP-0128`
- User-visible control point before next step:
  after this step lands, the next bounded move should come from concrete post-lookup progress on the two QEMU lanes
