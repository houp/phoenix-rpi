# Current Step

## Metadata

- Step ID: `STEP-0011`
- Title: Extend kernel AArch64 DTB parsing for first `virt`-style nodes
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- extend the kernel AArch64 DTB parser just enough to recognize the first non-ZynqMP `virt`-style nodes needed for a future generic QEMU lane, while preserving the existing `aarch64a53-zynqmp` path

## Scope

In scope:

- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- recognize root-level or shallow `intc@...` GIC nodes used by local QEMU `virt`
- recognize root-level or shallow `pl011@...` serial nodes used by local QEMU `virt`
- keep the existing `serial@...` and ZynqMP-compatible parsing behavior intact
- validate that the existing `aarch64a53-zynqmp-qemu` build still succeeds after the parser change

Out of scope:

- adding a new QEMU target
- adding PL011 console code
- adding generic timer code
- changing `plo`
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- copied-buildroot AArch64 validation workflow
- tracking files and manifest updates after validation

## Acceptance Criteria

- the kernel DTB parser recognizes `virt`-style `intc@...` and `pl011@...` node names in addition to the existing ZynqMP-oriented names
- the change stays small and limited to DTB discovery logic
- `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project` still succeeds inside `phoenix-dev` using the copied buildroot

## Validation Plan

- Build:
  refresh the copied buildroot, then run the existing `aarch64a53-zynqmp-qemu` build path in `phoenix-dev`
- Emulator:
  inspect the local QEMU `virt` DTB shape and confirm the new node-name checks match it
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-platform-makefile-selection.md`

## Notes

- Risks:
  node-name recognition alone will not complete a generic QEMU lane, but it should be a safe preparatory step toward that lane
- Dependencies:
  completed AArch64 Makefile cleanup and a working AArch64 validation toolchain in `phoenix-dev`
- User-visible control point before next step:
  present the exact parser change, validation command, and resulting commit before moving into timer, console, or target-definition work
