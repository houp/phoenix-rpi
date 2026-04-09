# 2026-04-09 Pi 4 Slower LED Telemetry Protocol

## Summary

The first structured ACT-LED telemetry video proved that the checkpoint groups
were present on real hardware, but the timing was still too dense to decode
confidently from one ordinary phone recording.

The bounded response in this step was:

- keep the same checkpoint map
- slow the pulse widths and inter-group separators substantially
- remove the redundant leading gap from each stage emitter so the full `1..9`
  sequence still fits within about one minute

## Current Timing Contract

- about `0.4s` LED on per pulse
- about `0.4s` LED off between pulses inside one group
- about `2.0s` LED off between groups

The current checkpoint map remains:

- `1`: armstub primary-core entry
- `2`: armstub after early timer / GIC preparation
- `3`: armstub just before the fixed-address jump to `plo`
- `4`: earliest generic AArch64 `plo` `_start`
- `5`: `plo` EL3 path selected
- `6`: `plo` EL2 path selected
- `7`: `plo` EL1 path selected
- `8`: `plo` `start_common`
- `9`: `plo` core-0 branch to `_startc`

## Touched Repositories

- `phoenix-rtos-project`
  - `6a027c3` `project: slow pi4 armstub led telemetry`
- `plo`
  - `b050494` `aarch64: slow pi4 plo led telemetry`
- coordination repo

## Validation

### Build

- `./scripts/prepare-rpi4b-dtb.sh`
- `limactl shell -y phoenix-dev -- /bin/bash -lc 'cd /Users/witoldbolt/phoenix-rpi && ./scripts/prepare-buildroot.sh --copy-components /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy'`
- `limactl shell -y phoenix-dev -- /bin/bash -lc 'set -euo pipefail; export PATH=/home/witoldbolt.guest/phoenix-toolchains/aarch64-phoenix/bin:$PATH; cd /home/witoldbolt.guest/phoenix-buildroots/phoenix-rtos-project-copy; RPI4B_DTB_PATH=/tmp/rpi4b-dtb/bcm2711-rpi-4-b.dtb TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image'`

### Emulator

- `./scripts/qemu-shell-smoke.sh generic`
  - passed
- direct Pi 4 QEMU serial sanity on the real-device build:
  - reached `call: exec go!`
  - reached `go: enter`
  - reached `hal: jump exit el1`
  - reached `A3`
  - reached `KLM`
  - then reached the known later direct-QEMU `Exception #37`
- `/bin/bash /Users/witoldbolt/phoenix-rpi/scripts/qemu-rpi4b-hdmi-smoke.sh`
  - attempted twice
  - both runs failed in the helper harness with early `socat ... Connection refused`
  - not treated as a product regression for this step

### Image Assembly And Export

- `./scripts/assemble-rpi4b-bootfs.sh`
- `./scripts/assemble-rpi4b-bootfs-img.sh`
- `./scripts/assemble-rpi4b-sdimg.sh`
- `./scripts/export-rpi4b-sdimg.sh`
- `./scripts/verify-rpi4b-sdimg.sh`
- exported host image:
  - `/Users/witoldbolt/phoenix-rpi/artifacts/rpi4b/rpi4b-sd.img`
  - SHA-256: `4698611f2231fd5508e6eddeed25a24147701ce3efc209371425ea75d502f23e`

## Notes

- The previous timing was already visible on real hardware, but it produced a
  sequence that was too dense to classify confidently from one phone clip.
- The slower protocol now requires a longer operator capture window:
  record at least `70` seconds and preferably `90` seconds from power-on.
