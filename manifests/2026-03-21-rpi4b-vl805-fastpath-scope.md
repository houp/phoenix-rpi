# Pi 4 VL805 Fast-Path Scope

Date: `2026-03-21`

## Step

- `STEP-0327` Scope the smallest first direct downstream endpoint-readback step

## Repositories

- coordination repo

## Summary

- reviewed the current Phoenix PCIe transport state against Circle's Pi 4 xHCI
  path
- selected the narrowest practical discovery contract for the first Pi 4 xHCI
  work:
  treat the downstream USB controller as the single expected VL805 endpoint at
  `bus 1 / slot 0 / func 0` with PCI class code `0x0c0330`
- explicitly chose the Pi 4 fast path over inventing a generic AArch64 PCI
  discovery service first

## Design Notes

- Circle's Pi 4 xHCI path uses:
  - `XHCI_PCIE_BUS = 1`
  - `XHCI_PCIE_SLOT = 0`
  - `XHCI_PCIE_FUNC = 0`
  - `XHCI_PCI_CLASS_CODE = 0x0c0330`
  - a fixed MMIO view through the outbound PCIe window after device enablement
- that is a valid next bounded step here because:
  - Pi 4 currently exposes a single downstream PCIe USB controller in the
    intended fast path
  - the current Phoenix AArch64 path has no IA32-style kernel PCI discovery API
  - a generic PCI discovery service would widen the scope before the first USB
    keyboard goal

## Validation

- source inspection against:
  - `external/circle/include/circle/usb/xhciconfig.h`
  - `external/circle/include/circle/bcm2711.h`
  - `external/circle/lib/usb/xhcidevice.cpp`
  - `sources/phoenix-rtos-devices/pcie/server/pcie.c`
  - `sources/phoenix-rtos-project/_projects/aarch64a72-generic-rpi4b/board_config.h`

## Next Logical Step

- implement the board-level Pi 4 VL805 fast-path constants needed by later xHCI
  code

