# BCM2711 Host Init Scope

Date: `2026-03-21`

## Step

- `STEP-0315` Scope the smallest BCM2711 host-bridge initialization step

## Repositories

- coordination repo

## Summary

- reviewed the gap between the new indexed config-space backend and real
  BCM2711 host-bridge operation
- selected the smallest useful init slice as:
  reset sequencing, revision read, and early `MISC_CTRL` preparation
- explicitly kept outbound windows, link-up checks, RC-mode checks, MSI, xHCI,
  and downstream enumeration out of scope

## Why This Comes Next

- config-space access mechanics alone do not imply that the host bridge is out
  of reset or prepared for meaningful downstream transactions
- Circle's sequence shows that reset release, early SerDes preparation, and
  `MISC_CTRL` programming happen before outbound-window setup and link-up
  verification
- this is therefore the smallest host-side bring-up move that materially
  reduces the gap without pretending the PCIe link is already usable

## References

- `external/circle/lib/bcmpciehostbridge.cpp`
- `sources/phoenix-rtos-devices/pcie/server/pcie.c`

## Validation

- source review only
- no code changes in this step

## Next Logical Step

- implement the backend-local BCM2711 reset/revision/basic-`MISC_CTRL`
  preparation hook and validate that it preserves the current compile lanes
