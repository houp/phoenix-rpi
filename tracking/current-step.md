# Current Step

## Metadata

- Step ID: `STEP-0068`
- Title: Define stdout-path-aware serial-selection fix for generic AArch64 kernel
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- define the smallest clean fix that makes the generic AArch64 kernel choose the correct early console UART on QEMU `virt,secure=on`

## Scope

In scope:

- inspect the generic AArch64 DTB serial API and generic kernel console selection
- compare that selection logic with the local `virt,secure=on` DTB ordering and `chosen.stdout-path`
- choose the narrowest clean fix for early kernel console selection

Out of scope:

- broad generic kernel bring-up changes
- Raspberry Pi-specific code
- implementing the fix in this planning step
- `phoenix-rtos-tests` target additions

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

- the result names the smallest concrete DTB or console-selection fix to apply next
- the result explains why that fix is preferred over broader kernel-entry instrumentation
- the step remains planning-only

## Validation Plan

- Review:
  inspect the DTB serial enumeration path, generic console init, and the local QEMU `stdout-path` evidence
- Build:
  not applicable
- Emulator:
  not applicable
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-generic-qemu-dtb-fix.md`

## Notes

- Risks:
  the result must stay as one serial-selection planning step and must not silently turn into broader kernel instrumentation or AArch64 bring-up
- Dependencies:
  completed implementation step `STEP-0067`
- User-visible control point before next step:
  after this planning step lands, the next slice should be the selected first stdout-path-aware console-selection fix
