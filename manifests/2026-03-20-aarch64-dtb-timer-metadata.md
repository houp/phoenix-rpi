# Manifest: AArch64 DTB Timer Metadata Step

- Date: `2026-03-20`
- Step: `STEP-0013`
- Result: `completed`

## Scope

- extend `phoenix-rtos-kernel/hal/aarch64/dtb.c` and `hal/aarch64/dtb.h` so the AArch64 DTB parser exposes architectural timer interrupt metadata from the root-level `timer` node
- keep the step preparatory only: no generic runtime timer code and no new target definitions
- validate that the existing `aarch64a53-zynqmp-qemu` build still succeeds in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `fbc99699` (`aarch64: parse timer interrupts from dtb`)

## Validation

- Inspected the local QEMU `virt` timer node in `phoenix-dev`:
  `timer { interrupts = <0x01 0x0d 0x104 0x01 0x0e 0x104 0x01 0x0b 0x104 0x01 0x0a 0x104>; ... }`
- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The AArch64 DTB API now exposes four architectural timer interrupt slots in binding order:
  - secure physical
  - non-secure physical
  - virtual
  - hypervisor
- There is still no runtime generic ARM timer implementation in the current Phoenix AArch64 kernel or `plo`.
- There is also still no PL011 implementation in the current Phoenix AArch64 tree.

## Selected Next Step

- define the first runtime-oriented kernel follow-up after the DTB preparation steps, with explicit comparison of:
  - a generic ARM architectural timer implementation path
  - a broader generic AArch64 platform-target split

## Rationale For The Next Step

- The DTB layer is now sufficiently prepared that the next change will need to introduce new runtime code or new target/build structure.
- That next move should be bounded first, so the work remains reviewable and does not silently widen from a parser-only series into a broad generic-QEMU port.
