# Pi 4 Re-freeze Restored X3NO Baseline

## Summary

- Date: `2026-04-18`
- Step name: `STEP-0517 Re-freeze the restored Pi 4 X3NO baseline after the failed O-to-P re-split`
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
  - the failed `U / V / W / Z / Y / P` post-MMU re-split was removed from the
    active kernel tree
  - the exported image is once again built from the last objectively better
    `... X3NO` lineage

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| phoenix-rtos-kernel | `https://github.com/phoenix-rtos/phoenix-rtos-kernel` | `pi4-dev` | `a4883d37` | changed and committed |
| plo | `https://github.com/phoenix-rtos/plo` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-devices | `https://github.com/phoenix-rtos/phoenix-rtos-devices` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-filesystems | `https://github.com/phoenix-rtos/phoenix-rtos-filesystems` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-build | `https://github.com/phoenix-rtos/phoenix-rtos-build` | `pi4-dev` | unchanged in this step | unchanged |
| phoenix-rtos-project | `https://github.com/phoenix-rtos/phoenix-rtos-project` | `pi4-dev` | `e8f794f` | unchanged in this step |
| phoenix-rtos-tests | `https://github.com/phoenix-rtos/phoenix-rtos-tests` | `pi4-dev` | unchanged in this step | unchanged |

## Validation Evidence

- Emulator:
  - `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`: pass
  - `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`:
    pass
  - `./scripts/qemu-shell-smoke.sh rpi4b`: inconclusive, helper hung without a
    final pass/fail transcript
- Hardware:
  - last better seam proof still remains:
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-222500.log`
  - failed re-split proof:
    `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b-uart/rpi4b-uart-20260418-234332.log`
- Image or boot-tree location:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256 `576bacf524d115f8f99361d0434eac32a92d0f1354f8169fb5c7fa24502f39d8`

## Notes

- The broad `./scripts/rebuild-rpi4b-fast.sh --scope core --qemu-sanity`
  helper still captured only the short `A3 / KLM` tail. That remains a known
  helper limitation, not a proof that the rollback baseline itself regressed in
  QEMU.
- The explicit Pi 4 HDMI smoke remained green on the refreshed rollback image.
- The next hardware retry should answer only one question:
  - does the re-frozen image return the board to `... X3NO`
  - if yes, the next diagnostic must be safer than the regressing `U..P` split
