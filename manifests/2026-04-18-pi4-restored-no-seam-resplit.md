# Pi 4 Restored-NO Seam Re-split

## Summary

- Date: `2026-04-18`
- Step name: `STEP-0516 Classify the restored Pi 4 O-to-P seam from the last better baseline`
- Scope:
  - `phoenix-rtos-kernel/hal/aarch64/_init.S`
  - coordination-repo tracking updates
- Validation lanes used:
  - `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`
  - `./scripts/qemu-shell-smoke.sh rpi4b`
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`
  - `./scripts/export-rpi4b-sdimg.sh`
  - `./scripts/verify-rpi4b-sdimg.sh`
- Result:
  - the restored `... X3NO` hardware baseline was **not** preserved on the
    first real-board retry
  - the added `U V W Z Y P` split regressed the hardware seam back to `... X3`
  - this step is therefore a negative result and should not be reused as the
    active image

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| phoenix-rtos-kernel | `https://github.com/phoenix-rtos/phoenix-rtos-kernel` | `pi4-dev` | `5f3bf75e` | changed and committed |
| plo | `https://github.com/phoenix-rtos/plo` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-devices | `https://github.com/phoenix-rtos/phoenix-rtos-devices` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-filesystems | `https://github.com/phoenix-rtos/phoenix-rtos-filesystems` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-build | `https://github.com/phoenix-rtos/phoenix-rtos-build` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-project | `https://github.com/phoenix-rtos/phoenix-rtos-project` | `pi4-dev` | `e8f794f` | unchanged in this step |
| phoenix-rtos-tests | `https://github.com/phoenix-rtos/phoenix-rtos-tests` | `pi4-dev` | unchanged in this step | unchanged |

## Validation Evidence

- Emulator:
  - `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
  - `./scripts/qemu-shell-smoke.sh rpi4b`: pass
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`: pass
- Hardware:
  - restored baseline proof before the re-split:
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-222500.log`
  - disproving re-split proof:
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-234332.log`
  - observed regression tail:
    - `A2`
    - `KLM`
    - `X1`
    - `X2`
    - `X3`
- UART log location:
  - the re-split image should no longer be used as the active baseline
- Image or boot-tree location:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `ff1b0ca7b4bb89f4f8812537750487566160fc4e583368748976f80b4c200cb4`

## Notes

- New constraints discovered:
  - `... X3NO` is the farthest real-board UART seam actually proven in the
    project history so far
  - apparent later progress in manifests such as
    `KLMNOPQRSconsole: pl011 init done`, `main: hal init done`, kernel banner,
    and later `Exception #37` are QEMU-only milestones, not later hardware
    UART milestones
  - that means a deeper rollback would discard the strongest hardware-backed
    checkpoint instead of recovering a demonstrably better one
  - adding fine post-MMU virtual-UART markers inside `_core_0_virtual` can
    itself perturb the restored `X3NO` seam enough to drop the board back to
    `X3`
- Docs updated:
  - `/Users/witoldbolt/phoenix-rpi/docs/status.md`
  - `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
  - `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`
- Next smallest task:
  - revert the re-split
  - rebuild from the restored `X3NO` lineage
  - verify that hardware returns to `... X3NO` before choosing a safer
    diagnostic
