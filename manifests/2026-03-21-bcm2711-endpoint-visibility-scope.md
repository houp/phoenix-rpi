# BCM2711 Endpoint Visibility Scope

Date: `2026-03-21`

## Step

- `STEP-0323` Scope the smallest first endpoint-visibility and enumeration step

## Repositories

- coordination repo

## Summary

- reviewed the remaining gap between the new bridge-exposure step and a
  meaningful first downstream endpoint sighting
- selected one additional bridge-side capability slice before any direct
  downstream claim:
  root-bridge parity plus PCIe root-control CRS software visibility
- kept full enumeration, endpoint policy, MSI, and xHCI explicitly out of
  scope

## Design Notes

- the current Phoenix scan path already walks bus `1` once the root bridge
  exposes that bus, so the next smallest missing step is not “add scanning”
- the closest bounded follow-up, and the last piece still missing from the
  current Circle `enable_bridge()` pattern, is:
  - `PCI_BRIDGE_CONTROL` parity enable
  - `BRCM_PCIE_CAP_REGS + PCI_EXP_RTCTL` CRS software visibility enable
- that step is still bridge-local and does not overclaim that a downstream
  endpoint is already visible

## Validation

- source inspection against:
  - `sources/phoenix-rtos-devices/pcie/server/pcie.c`
  - `external/circle/lib/bcmpciehostbridge.cpp`
  - `external/circle/include/circle/pci_regs.h`

## Next Logical Step

- implement the bounded BCM2711 bridge-capability enablement step before any
  direct downstream endpoint readback work
