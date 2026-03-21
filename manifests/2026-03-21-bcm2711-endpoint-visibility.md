# BCM2711 Endpoint Visibility

Date: `2026-03-21`

## Step

- `STEP-0324` Implement the smallest bridge-capability enablement step for
  first downstream endpoint visibility

## Repositories

- `phoenix-rtos-devices` `d6fe26d`
- coordination repo

## Summary

- completed the last bounded bridge-side slice borrowed from Circle
  `enable_bridge()` before any downstream endpoint-specific claim:
  root-bridge parity enablement and PCIe root-control CRS software visibility
- kept the change behind the existing BCM2711 link / RC-mode gate
- kept enumeration policy, MSI, xHCI, and endpoint-driver work explicitly out
  of scope

## Key Files

- `sources/phoenix-rtos-devices/pcie/server/pcie.c`

## Design Notes

- the new step is intentionally small:
  it only prepares the root bridge to expose downstream config-space state more
  faithfully before the first direct endpoint readback
- the implementation follows the current Circle `enable_bridge()` pattern:
  - `PCI_BRIDGE_CONTROL` parity bit
  - `BRCM_PCIE_CAP_REGS + PCI_EXP_RTCTL` CRS software visibility bit when the
    capability ID confirms a PCIe capability block

## Validation

Validated in `phoenix-dev`:

Preserved Xilinx compile lane:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/zynq-pcie-endpt.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a53-zynqmp-qemu \
  CPPFLAGS="-I$PWD/_projects/aarch64a53-zynqmp-qemu" pcie
```

Touched Pi 4 compile lane and regression build:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-pcie-endpt.XXXXXX)
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

- the current backend still lacks one direct, bounded downstream config-space
  readback step that would show whether bus `1` device visibility is beginning
  to work meaningfully

## Next Logical Step

- scope the smallest first downstream endpoint readback step before widening
  into broader enumeration or xHCI
