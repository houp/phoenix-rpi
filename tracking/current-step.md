# Current Step

## Metadata

- Step ID: `STEP-0069`
- Title: Implement stdout-path-aware generic AArch64 console selection
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- implement the smallest clean fix that makes the generic AArch64 kernel choose the correct early console UART on QEMU `virt,secure=on` and rerun the smoke lane

## Scope

In scope:

- add an AArch64 DTB helper that prefers `chosen.stdout-path` when it identifies a parsed PL011 serial node
- switch the generic kernel console to use that helper
- rebuild the generic kernel plus project/image lane in `phoenix-dev`
- rerun `timeout 10s ./scripts/aarch64a53-generic-qemu.sh`
- record the earliest post-fix result

Out of scope:

- broad generic DTB parser redesign
- non-generic AArch64 console changes
- Raspberry Pi-specific code
- `phoenix-rtos-tests` target additions
- fixing any later runtime issue beyond documenting it

## Expected Repositories

- coordination repo
- `phoenix-rtos-project`
- `plo`

## Expected Files Or Subsystems

- `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- `phoenix-rtos-kernel/hal/aarch64/dtb.h`
- `phoenix-rtos-kernel/hal/aarch64/generic/console.c`
- `docs/status.md`
- tracking files and manifest updates for this step
- smoke output captured from the copied buildroot in `phoenix-dev`

## Acceptance Criteria

- the DTB layer exposes a preferred console serial based on `chosen.stdout-path` when available
- the generic kernel console uses that helper instead of `serials[0]`
- the rerun records whether the kernel now reaches visible output or what the next earliest runtime failure is

## Validation Plan

- Review:
  inspect the DTB serial enumeration path, generic console init, and the local QEMU `stdout-path` evidence as needed during result analysis
- Build:
  rebuild the generic kernel and project/image artifacts in `phoenix-dev`
- Emulator:
  run `timeout 10s ./scripts/aarch64a53-generic-qemu.sh` inside the copied buildroot in `phoenix-dev`
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-stdout-path-scope.md`

## Notes

- Risks:
  the result must stay as one DTB-helper plus generic-console selection step and must not silently turn into broader kernel instrumentation or AArch64 bring-up
- Dependencies:
  completed implementation step `STEP-0068`
- User-visible control point before next step:
  after this rerun lands, the next slice should be the smallest runtime-fix step implied by the earliest observed post-console-selection result
