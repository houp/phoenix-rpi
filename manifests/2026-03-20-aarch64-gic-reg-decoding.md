# Manifest: AArch64 GIC `reg` Decoding Step

- Date: `2026-03-20`
- Step: `STEP-0012`
- Result: `completed`

## Scope

- extend `phoenix-rtos-kernel/hal/aarch64/dtb.c` so interrupt-controller `reg` decoding handles both:
  - the current 12-byte tuple layout used by the existing ZynqMP-oriented path
  - the 16-byte tuple layout exposed by local QEMU `virt`
- preserve the current `aarch64a53-zynqmp-qemu` build behavior

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `28bfed69` (`aarch64: decode gic reg tuple layouts`)

## Validation

- Inspected the local QEMU `virt` GIC node in `phoenix-dev`:
  `intc@8000000 { reg = <0x00 0x8000000 0x00 0x10000 0x00 0x8010000 0x00 0x10000>; ... }`
- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The current parser now handles both the older 12-byte tuple assumption and the 16-byte tuple layout observed in `virt`.
- `interrupts_gicv2.c` only needs the distributor and CPU-interface base addresses from the DTB, so this remains a narrowly scoped parser improvement.

## Selected Next Step

- extend the AArch64 DTB parser so it exposes architectural timer interrupt metadata from the root-level `timer` node without introducing the generic runtime timer implementation yet

## Rationale For The Next Step

- There is currently no reusable PL011 or ARM architectural timer implementation in either `phoenix-rtos-kernel` or `plo`.
- A parser-only timer metadata step stays narrow, is useful for future generic AArch64 timer work, and avoids mixing DTB parsing with runtime timer bring-up in one patch.
