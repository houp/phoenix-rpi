# TD-16 Early Page-Table Invalidation Integration State

## Summary

- Date: 2026-05-04
- Step name: TD-16 step 3 — restore early page-table invalidation
- Scope: kernel AArch64 bootstrap only
- Validation lanes used: Pi 4 QEMU shell smoke, generic AArch64 QEMU shell
  smoke, real Raspberry Pi 4 netboot/UART
- Result: passed; real Pi 4 still reaches `(psh)%`

## Repositories

| Repository | Remote URL | Branch | Commit SHA | Status |
| --- | --- | --- | --- | --- |
| phoenix-rtos-kernel | https://github.com/phoenix-rtos/phoenix-rtos-kernel.git | agent/rpi4-program-reloc | `5e727dccf89e0dab8ccbfaa81765ab910a3c4184` | changed |
| plo | https://github.com/phoenix-rtos/plo.git | codex/common-aarch64-platform-makefiles | `61927ba0` | unchanged |
| phoenix-rtos-devices | https://github.com/phoenix-rtos/phoenix-rtos-devices.git | master | `3ee47026` | unchanged |
| phoenix-rtos-filesystems | https://github.com/phoenix-rtos/phoenix-rtos-filesystems.git | master | `18840437` | unchanged |
| phoenix-rtos-build | https://github.com/phoenix-rtos/phoenix-rtos-build.git | master | `044ae924` | unchanged |
| phoenix-rtos-project | https://github.com/phoenix-rtos/phoenix-rtos-project.git | master | `21bda559` | unchanged |
| phoenix-rtos-tests | https://github.com/phoenix-rtos/phoenix-rtos-tests.git | master | `f7978fcc` | unchanged |
| libphoenix | https://github.com/phoenix-rtos/libphoenix.git | master | `3c76bba5` | unchanged |
| phoenix-rtos-utils | https://github.com/phoenix-rtos/phoenix-rtos-utils.git | master | `da2f5415` | unchanged |

## Validation Evidence

- Emulator:
  - `./scripts/qemu-shell-smoke.sh rpi4b` reached `(psh)% help`.
  - `./scripts/qemu-shell-smoke.sh generic` reached `(psh)% help`.
- Hardware:
  - `./scripts/test-cycle-netboot.sh --label td16-early-pt-inval --capture-secs 600 --dhcp-wait-secs 90`
    reached `(psh)%` on real Raspberry Pi 4.
- UART log location:
  - `artifacts/rpi4b-uart/rpi4b-uart-20260503-221342-netboot-td16-early-pt-inval.log`
- Image:
  - `artifacts/rpi4b/rpi4b-sd.img`
  - SHA256: `0f6dc1a9e8254d9c42f41d6ee308eff074a9a6a2e0810cc1fa25044d9c260115`

## Notes

- New constraints discovered:
  - The restored early page-table invalidation is safe after the TD-16 TTBR0
    alias cleanup.
  - It does not improve speed; TD-16 loop deltas remain around `0x883e**`,
    so caches are still disabled as expected.
- Warnings:
  - No build/export/DTB/image warnings.
  - Real Pi firmware emitted expected netboot-path misses and HDMI1 EDID/DSI
    messages while HDMI0 was active.
  - UART helper selected `picocom` and printed `STDIN is not a TTY`; capture
    still completed and the log is valid.
- Docs updated:
  - `docs/status.md`
  - `tracking/current-step.md`
  - `tracking/step-history.md`
  - `docs/TEMPORARY-FIXES-AND-FUTURE-CLEANUP.md`
- Next smallest task:
  - Attempt the early `SCTLR_EL1.M|C|I` transition in the Linux/FreeBSD shape,
    or first add a no-call early ESR/ELR/FAR dump if the current exception
    path is still too fragile for cache-enable fault diagnosis.
