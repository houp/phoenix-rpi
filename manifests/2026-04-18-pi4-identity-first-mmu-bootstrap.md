# Pi 4 Identity-First MMU Bootstrap

## Summary

- Date: `2026-04-18`
- Step name: `STEP-0512 Rework Pi 4 kernel MMU bring-up to an identity-first bootstrap path`
- Scope:
  - `phoenix-rtos-kernel/hal/aarch64/_init.S`
  - coordination-repo tracking and reference updates
- Validation lanes used:
  - `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`
  - `./scripts/qemu-shell-smoke.sh rpi4b`
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`
  - `./scripts/export-rpi4b-sdimg.sh`
  - `./scripts/verify-rpi4b-sdimg.sh`
- Result:
  - the Pi 4 kernel no longer enables MMU and immediately depends on
    higher-half execution; it now continues first in the TTBR0 identity alias,
    then enables `TTBR1`, then branches to `_core_0_virtual`
  - all strong QEMU and image-validation lanes passed

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| phoenix-rtos-kernel | `https://github.com/phoenix-rtos/phoenix-rtos-kernel` | `pi4-dev` | `6cd294fd` | changed and committed |
| plo | `https://github.com/phoenix-rtos/plo` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-devices | `https://github.com/phoenix-rtos/phoenix-rtos-devices` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-filesystems | `https://github.com/phoenix-rtos/phoenix-rtos-filesystems` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-build | `https://github.com/phoenix-rtos/phoenix-rtos-build` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-project | `https://github.com/phoenix-rtos/phoenix-rtos-project` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-tests | `https://github.com/phoenix-rtos/phoenix-rtos-tests` | `pi4-dev` | unchanged in this step | unchanged |

## Validation Evidence

- Emulator:
  - `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
  - `./scripts/qemu-shell-smoke.sh rpi4b`: pass
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`: pass
- Hardware:
  - not yet executed for this new identity-first image
- UART log location:
  - next expected real-board proof should replace the previous
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-004543.log`
- Image or boot-tree location:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `5ac0d1290867556a78fe19bad048b1cfe98e8c5328053c2d588ed0d8691006fe`

## Notes

- New constraints discovered:
  - the stale-image theory was already disproved before this step; repeated
    `3C` hangs were coming from a real runtime failure, not an export mismatch
  - the current Pi 4 kernel path was still the outlier versus Linux, Circle,
    `rpi4-bare-metal`, and `rpi-os` because it enabled `TTBR1` before MMU-on
    and immediately depended on higher-half execution
  - the new identity-first path intentionally matches the simpler structure
    used by known-good references: first continuation in the identity alias,
    higher-half activation second
- Docs updated:
  - `/Users/witoldbolt/phoenix-rpi/docs/status.md`
  - `/Users/witoldbolt/phoenix-rpi/docs/source-artifacts.md`
  - `/Users/witoldbolt/phoenix-rpi/tracking/current-step.md`
  - `/Users/witoldbolt/phoenix-rpi/tracking/step-history.md`
- Next smallest task:
  - flash the new image on real hardware
  - capture a UART log
  - verify whether the board finally moves beyond the long-standing `3C`
    boundary
