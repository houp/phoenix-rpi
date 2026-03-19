# Manifest: AArch64 DTB Node Recognition Step

- Date: `2026-03-20`
- Step: `STEP-0011`
- Result: `completed`

## Scope

- extend `phoenix-rtos-kernel/hal/aarch64/dtb.c` so the AArch64 kernel DTB parser recognizes the first non-ZynqMP `virt`-style node names needed for a future generic QEMU lane
- preserve the existing ZynqMP-oriented `serial@...` and `interrupt-controller@...` matching path
- validate that the existing `aarch64a53-zynqmp-qemu` build still succeeds in `phoenix-dev`

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `66c65c14` (`aarch64: recognize generic dtb node names`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- The kernel DTB parser now recognizes shallow `pl011@...` serial nodes in addition to the existing `serial@...` nodes.
- The kernel DTB parser now recognizes shallow `intc@...` interrupt-controller nodes in addition to the existing ZynqMP `interrupt-controller@...` path under `amba_apu`.
- The current local `virt` DTB shape matches those new node names, so the parser is now better aligned with a future generic QEMU lane and later Raspberry Pi DTB work.

## New Constraint Identified

- QEMU `virt` uses a GIC `reg` property with 16-byte tuples:
  `reg = <0x00 0x8000000 0x00 0x10000 0x00 0x8010000 0x00 0x10000>;`
- The current `dtb_parseInterruptController()` logic still assumes the older 12-byte tuple layout when extracting the second GIC base address, so the next smallest safe follow-up step should stay in `hal/aarch64/dtb.c` and generalize GIC `reg` decoding before broader timer, console, or target-definition work.
