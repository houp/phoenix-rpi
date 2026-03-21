# Current Step

## Metadata

- Step ID: `STEP-0255`
- Title: Scope cleanup of obsolete console-path probes
- Status: `planned`
- Date: `2026-03-21`
- Milestone / phase: `Phase 1`

## Objective

- identify the smallest cleanup set for the console-path probes that are no
  longer needed now that both fast-lane QEMU targets reach `(psh)%`

## Scope

In scope:

- inspect the currently committed `psh`, libphoenix, and kernel console-path
  traces added during the `/dev/console` investigation
- identify which ones were only diagnostic and should now be removed
- keep the selected cleanup set small and reviewable

Out of scope:

- changing the new `/dev` bind startup logic
- unrelated boot-trace cleanup
- new shell features
- real hardware work

## Expected Repositories

- coordination repo
- `sources/phoenix-rtos-utils`
- `sources/libphoenix`
- `sources/phoenix-rtos-kernel`

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

- one small cleanup set is identified
- the selected cleanup does not remove still-needed validation signal
- the result is captured in one manifest and the next implementation step

## Validation Plan

- Analysis:
  source review of the currently committed console-path probes
- Emulator:
  not required unless the necessity of one probe remains ambiguous
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-21-aarch64-fastlane-dev-bind.md`

## Notes

- Risks:
  keep the cleanup narrow and avoid removing broader bring-up visibility that is
  still actively useful
- Dependencies:
  completed `STEP-0254` prompt-reaching `/dev` bind fix
- Source reminder:
  current prompt-reaching logs already prove the old `/dev/console` issue is
  resolved
- User-visible control point before next step:
  after this scope step lands, the next change should be cleanup-oriented, not a
  new subsystem jump
