# BCM2711 Host Init Prepare

Date: `2026-03-21`

## Step

- `STEP-0316` Implement the first BCM2711 host-bridge preparation hook

## Repositories

- `phoenix-rtos-devices` `cfa3e06`
- coordination repo

## Summary

- added the first backend-local BCM2711 host-bridge preparation hook behind the
  indexed config-space backend
- that hook now performs:
  - bridge reset assert/deassert
  - PERST assert
  - SerDes IDDQ clear
  - revision read
  - early `MISC_CTRL` preparation for `SCB_ACCESS_EN`,
    `CFG_READ_UR_MODE`, and 128-byte burst policy
- kept outbound windows, PERST release, link-up checks, RC-mode checks, MSI,
  xHCI, and downstream enumeration out of scope

## Key Files

- `sources/phoenix-rtos-devices/pcie/server/pcie.c`

## Validation

Validated in `phoenix-dev`:

Preserved Xilinx compile lane:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/zynq-pcie-init.XXXXXX)
./scripts/prepare-buildroot.sh --copy-components "$tmpdir"
cd "$tmpdir"
make -C phoenix-rtos-devices TARGET=aarch64a53-zynqmp-qemu \
  CPPFLAGS="-I$PWD/_projects/aarch64a53-zynqmp-qemu" pcie
```

Touched Pi 4 compile lane and regression build:

```sh
export PATH="$HOME/phoenix-toolchains/aarch64-phoenix/bin:$PATH"
cd /Users/witoldbolt/phoenix-rpi
tmpdir=$(mktemp -d ~/phoenix-buildroots/pi4-pcie-init.XXXXXX)
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

- the BCM2711 backend is still not performing outbound-window setup, PERST
  release, link-up checks, RC-mode verification, or downstream enumeration

## Next Logical Step

- scope the smallest BCM2711 link-bring-up step after this early host-bridge
  preparation hook
