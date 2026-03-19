# Current Step

## Metadata

- Step ID: `STEP-0012`
- Title: Generalize kernel AArch64 GIC `reg` decoding for `virt`-style tuples
- Status: `in_progress`
- Date: `2026-03-20`
- Milestone / phase: `Phase 1`

## Objective

- extend the kernel AArch64 DTB parser so interrupt-controller `reg` decoding handles both the existing ZynqMP-oriented layout and the 16-byte tuple layout used by local QEMU `virt`, while preserving the existing `aarch64a53-zynqmp` path

## Scope

In scope:

- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- update `phoenix-rtos-kernel/hal/aarch64/dtb.c`
- extend `dtb_parseInterruptController()` to decode the first two GIC base addresses from both:
  - the current 12-byte tuple layout used by the existing code path
  - the 16-byte tuple layout observed in local QEMU `virt`
- keep the existing GIC discovery and build behavior intact for `aarch64a53-zynqmp`
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

- the kernel DTB parser decodes GIC distributor and CPU-interface base addresses from both the current 12-byte tuple layout and the 16-byte tuple layout used by local QEMU `virt`
- the change stays small and limited to DTB interrupt-controller parsing logic
- `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project` still succeeds inside `phoenix-dev` using the copied buildroot

## Validation Plan

- Build:
  refresh the copied buildroot, then run the existing `aarch64a53-zynqmp-qemu` build path in `phoenix-dev`
- Emulator:
  inspect the local QEMU `virt` GIC `reg` property and confirm the parser covers the observed 16-byte tuple layout
- Hardware:
  not applicable

## Rollback / Baseline

- Known-good manifest or commit set:
  `manifests/2026-03-20-aarch64-dtb-node-recognition.md`

## Notes

- Risks:
  tuple-layout recognition alone will not complete a generic QEMU lane, but it should keep the next parser change narrow and source-backed
- Dependencies:
  completed AArch64 node-name recognition step and a working AArch64 validation toolchain in `phoenix-dev`
- User-visible control point before next step:
  present the exact GIC parsing change, validation command, and resulting commit before moving into timer, console, or target-definition work
