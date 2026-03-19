# Current Step

## Metadata

- Step ID: `STEP-0013`
- Title: Expose architectural timer interrupts from the AArch64 DTB parser
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- extend the kernel AArch64 DTB parser so it exposes architectural timer interrupt metadata from the root-level `timer` node, while preserving the existing `aarch64a53-zynqmp` path and without introducing a runtime generic timer implementation yet

## Scope

In scope:

- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- update `phoenix-rtos-kernel/hal/aarch64/dtb.h`
- recognize the root-level `timer` node used by local QEMU `virt`
- parse the `interrupts` property from that node into a small AArch64 DTB timer structure
- keep the change preparatory only: no new runtime timer code, no target changes
- validate that the existing `aarch64a53-zynqmp-qemu` build still succeeds after the parser change

Out of scope:

- adding a new QEMU target
- adding PL011 console code
- adding generic timer runtime code
- changing `plo`
- Raspberry Pi-specific code

## Expected Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Expected Files Or Subsystems

- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.c`
- `sources/phoenix-rtos-kernel/hal/aarch64/dtb.h`
- copied-buildroot AArch64 validation workflow
- tracking files and manifest updates after validation

## Acceptance Criteria

- the AArch64 DTB parser exposes architectural timer interrupt metadata from the root-level `timer` node
- the change stays small and limited to DTB parsing and DTB API surface
- `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project` still succeeds inside `phoenix-dev` using the copied buildroot

## Validation Plan

- Build:
  refresh the copied buildroot, then run the existing `aarch64a53-zynqmp-qemu` build path in `phoenix-dev`
- Emulator:
  inspect the local QEMU `virt` `timer` node and confirm the parser covers the observed interrupt tuple layout
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-gic-reg-decoding.md`

## Notes

- Risks:
  timer metadata exposure alone will not complete a generic QEMU lane, but it should keep future timer work split into smaller reviewable patches
- Dependencies:
  completed AArch64 GIC parsing step and a working AArch64 validation toolchain in `phoenix-dev`
- User-visible control point before next step:
  present the exact DTB timer parsing change, validation command, and resulting commit before moving into runtime timer, console, or target-definition work
