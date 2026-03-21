# Current Step

## Metadata

- Step ID: `STEP-0256`
- Title: Remove obsolete console-path probes after the fast-lane fix
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- remove the smallest set of console-path diagnostic code that is no longer
  needed now that both fast-lane QEMU targets reach `(psh)%`

## Scope

In scope:

- remove the old `/dev/console` investigation traces from:
  - `psh`
  - `libphoenix`
  - kernel lookup / posix open paths
- keep the currently useful broader boot-band markers
- revalidate generic and Pi 4 QEMU prompt reachability after cleanup

Out of scope:

- wider boot-trace cleanup
- changing the new `/dev` bind startup fix
- new shell behavior
- real hardware work

## Expected Repositories

- `sources/phoenix-rtos-utils`
- `sources/libphoenix`
- `sources/phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-utils/psh/psh.c`
- `sources/libphoenix/unistd/file.c`
- `sources/phoenix-rtos-kernel/syscalls.c`
- `sources/phoenix-rtos-kernel/posix/posix.c`
- `docs/status.md`
- `manifests/`
- `tracking/current-step.md`
- `tracking/step-history.md`

## Acceptance Criteria

- the selected console-path probes are removed cleanly
- generic QEMU still reaches `(psh)%`
- Pi 4 QEMU still reaches `(psh)%`
- no unrelated diagnostic removals are mixed into the patch

## Validation Plan

- Build:
  copied-buildroot generic and Pi 4 rebuilds in `phoenix-dev`
- Emulator:
  generic QEMU first, then Pi 4 `raspi4b`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-console-probe-cleanup-scope.md`

## Notes

- Risks:
  do not remove broader bring-up markers that are still actively useful
- Dependencies:
  completed `STEP-0255` cleanup scoping
- Source reminder:
  the prompt-reaching fast lane has already superseded these console-path probes
- User-visible control point before next step:
  after this cleanup lands, the next steps can shift back to either automated
  shell smoke or new Pi 4 boot-completeness work
