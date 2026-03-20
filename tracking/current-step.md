# Current Step

## Metadata

- Step ID: `STEP-0089`
- Title: Add `psh` to the generic `user.plo`
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- add plain `psh` to the generic runtime image without introducing an `rc.psh` overlay yet

## Scope

In scope:

- update `_targets/aarch64a53/generic/user.plo.yaml`
- rebuild the required generic utils/project artifacts
- rerun the generic QEMU smoke lane

Out of scope:

- all upstream source changes
- Pi 4 board-specific code
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions

## Expected Repositories

- coordination repo
- `phoenix-rtos-devices`

## Expected Files Or Subsystems

- `phoenix-rtos-project/_targets/aarch64a53/generic/user.plo.yaml`
- comparable QEMU `user.plo` files
- `phoenix-rtos-devices/tty/pl011-tty/*`
- `docs/status.md`
- tracking files and manifest updates for this step
- generic QEMU smoke output
- generic utils packaging expectations

## Acceptance Criteria

- the generic `user.plo` now includes plain `psh`
- the needed generic artifacts are rebuilt and repackaged
- the generic QEMU smoke lane is rerun from the refreshed image

## Validation Plan

- Review:
  inspect the `user.plo` edit against comparable minimal console-plus-shell scripts
- Build:
  rebuild the required generic utils/project artifacts in `phoenix-dev`
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-psh-scope.md`

## Notes

- Risks:
  the result must stay as one `user.plo` runtime step and must not silently turn into `rc.psh` overlay work
- Dependencies:
  completed implementation step `STEP-0088`
- User-visible control point before next step:
  after this step lands, the next step should be chosen from the new smoke output
