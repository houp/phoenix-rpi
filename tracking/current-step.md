# Current Step

## Metadata

- Step ID: `STEP-0140`
- Title: Instrument `pl011-tty` retry wake-return visibility
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- determine whether the bounded retry path stalls inside `usleep(100000)` rather than on a second `lookup("devfs", ...)` call

## Scope

In scope:

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- keep the current bounded retry window
- add only the minimum raw UART marker needed immediately after `usleep(100000)` returns
- preserve the existing lookup / retry markers so the new output can be compared directly with `STEP-0138`
- validate on both the generic and Pi 4 DTB-backed QEMU lanes

Out of scope:

- broad `dummyfs` lifecycle refactoring
- loader-script or service-order changes
- broad `pl011-tty` redesign
- real-hardware-only validation
- Pi 5 or RP1 work
- `phoenix-rtos-tests` integration
- changes to the actual retry timing window
- broad refactoring of `create_dev()` semantics

## Expected Repositories

- `sources/phoenix-rtos-filesystems`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-devices/tty/pl011-tty/pl011-tty.c`
- relevant generic and Pi 4 QEMU smoke notes
- manifests and tracking updates for this implementation step

## Acceptance Criteria

- the generic lane exposes whether the retry loop wakes up after the first `usleep(100000)`
- the output distinguishes “sleep never returned” from “sleep returned and the second lookup blocked”
- neither lane regresses from current known-good startup output

## Validation Plan

- Review:
  confirm that the patch stays local to the current `pl011-tty` helper and only adds a post-sleep marker
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
  `manifests/2026-03-20-aarch64-proc-name-lookup-visibility.md`

## Notes

- Risks:
  avoid turning bounded visibility markers into long-lived logging churn
- Dependencies:
  completed `STEP-0138` kernel name-service visibility result
- User-visible control point before next step:
  after this step lands, the next bounded move should come from concrete wake-versus-second-lookup evidence rather than more indirect inference
