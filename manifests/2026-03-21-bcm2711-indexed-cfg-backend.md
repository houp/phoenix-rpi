# BCM2711 Indexed Config Backend

Date: `2026-03-21`

## Step

- `STEP-0314` Implement the compile-only BCM2711 indexed config-space backend

## Repositories

- `phoenix-rtos-devices` `63556bc`
- `phoenix-rtos-project` `c34f012`
- coordination repo

## Summary

- added the first BCM2711-specific PCIe config-space backend behind the current
  `pcie_cfgio_t` interface
- selected that backend for the Pi 4 A72 project through build settings and
  board constants
- kept the step compile-only: no link training, outbound windows, DMA windows,
  MSI, xHCI, or downstream-enumeration claims were added

## Key Files

- `sources/phoenix-rtos-devices/pcie/server/pcie.c`
- `sources/phoenix-rtos-devices/pcie/server/Makefile`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/build.project`
- `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`

## Design Notes

- the backend uses direct root-complex register access for bus `0`, slot `0`,
  function `0`
- downstream config-space accesses use the BCM2711 indexed registers:
  `PCIE_EXT_CFG_INDEX` and `PCIE_EXT_CFG_DATA`
- invalid bus-0 non-root accesses return all ones and ignore writes, matching
  the intended “no device present” semantics for the compile-only stage
- the Pi 4 project now exports `PCI_EXPRESS_BCM2711_INDEXED_CFG=y` and carries
  `PCIE_BCM2711_HOST_BASE` plus `PCIE_BCM2711_HOST_SIZE`

## Validation

Validated in `phoenix-dev`:

Preserved Xilinx compile lane:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/zynq-pcie-backend.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a53-zynqmp-qemu \
  CPPFLAGS="-I$PWD/_projects/aarch64a53-zynqmp-qemu" pcie
```

Observed result:

- the preserved Xilinx-targeted `pcie` server still compiles and links

Touched Pi 4 compile lane:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-pcie-backend.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a72-generic-rpi4b \
  PCI_EXPRESS_BCM2711_INDEXED_CFG=y \
  CPPFLAGS="-I$PWD/_projects/aarch64a72-generic-rpi4b" pcie
TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image
```

Observed result:

- the Pi 4 targeted `pcie` server now compiles and links with the BCM2711
  indexed backend selected
- a fresh full Pi 4 A72 build still succeeds from the same disposable buildroot

## Remaining Gap

- this step only provides config-space access mechanics
- real Pi 4 PCIe still needs BCM2711 host-bridge initialization and link bring-up
  before downstream enumeration or xHCI work can be claimed

## Next Logical Step

- scope the smallest BCM2711 host-bridge initialization step after the new
  indexed-config backend
