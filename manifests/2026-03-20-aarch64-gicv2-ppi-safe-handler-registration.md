# Manifest: AArch64 GICv2 PPI-Safe Handler Registration

- Date: `2026-03-20`
- Step: `STEP-0023`
- Result: `completed`

## Scope

- update common AArch64 GICv2 handler registration so it applies CPU targeting only to SPI interrupts
- preserve existing SPI behavior and validate the current `aarch64a53-zynqmp-qemu` build

## Touched Repositories

- `phoenix-rtos-kernel`
- coordination repo

## Upstream Commits

- `phoenix-rtos-kernel`: `2d542fd4` (`aarch64: avoid retargeting non-spi handlers`)

## Validation

- Refreshed the copied buildroot in `phoenix-dev`:
  `./scripts/prepare-buildroot.sh --copy-components`
- Rebuilt the existing AArch64 QEMU lane in `phoenix-dev`:
  `TARGET=aarch64a53-zynqmp-qemu ./phoenix-rtos-build/build.sh clean host core project`
- Build result: success

## Key Findings

- Common AArch64 GICv2 handler registration no longer pushes SGI or PPI handlers through the SPI-only CPU-targeting path.
- This removes one interrupt-layer assumption that would have conflicted with a future architectural timer IRQ delivered as a PPI.
- The larger timer blocker remains the wakeup-programming contract: non-CPU0 contexts can still request timer reprogramming through `hal_timerSetWakeup()`.

## Selected Next Step

- expose a targeted AArch64 SGI helper so later timer-path work can notify CPU 0 directly without depending only on broadcast IPIs
