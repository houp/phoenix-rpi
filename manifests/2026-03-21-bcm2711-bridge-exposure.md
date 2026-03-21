# BCM2711 Bridge Exposure

Date: `2026-03-21`

## Step

- `STEP-0322` Implement the smallest root-bridge memory-window and downstream-bus exposure step

## Repositories

- `phoenix-rtos-devices` `d7e5ec7`
- coordination repo

## Summary

- added the next bridge-side BCM2711 step:
  cache-line setup, bus-number exposure, bridge memory-window programming, and
  bridge command enablement on bus `0`
- kept that step gated behind the sampled BCM2711 link / RC-mode state
- kept endpoint-specific validation and xHCI explicitly out of scope

## Key Files

- `sources/phoenix-rtos-devices/pcie/server/pcie.c`

## Design Notes

- this step follows the current Circle `enable_bridge()` pattern closely, but
  keeps the implementation bounded to the minimum bridge-side exposure logic
- it does not claim that a downstream device is present or that enumeration has
  been validated on real hardware

## Validation

Validated in `phoenix-dev`:

Preserved Xilinx compile lane:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/zynq-pcie-bridge.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a53-zynqmp-qemu \
  CPPFLAGS="-I$PWD/_projects/aarch64a53-zynqmp-qemu" pcie
```

Touched Pi 4 compile lane and regression build:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-pcie-bridge.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a72-generic-rpi4b \
  PCI_EXPRESS_BCM2711_INDEXED_CFG=y \
  CPPFLAGS="-I$PWD/_projects/aarch64a72-generic-rpi4b" pcie
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Observed result:

- the preserved Xilinx-targeted `pcie` server still compiles and links
- the Pi 4 targeted `pcie` server still compiles and links
- a fresh full Pi 4 A72 build still succeeds from the same disposable buildroot

## Remaining Gap

- the current backend still lacks any validated first endpoint-visibility step
  that would show whether a downstream device is meaningfully present on bus `1`

## Next Logical Step

- scope the smallest first endpoint-visibility and enumeration step before any
  xHCI-specific work
