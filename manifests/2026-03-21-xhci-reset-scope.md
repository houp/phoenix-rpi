# STEP-0335: Scope the first xHCI controller-reset slice

## Date

- 2026-03-21

## Goal

- define the smallest controller-readiness slice that should follow the new
  Pi 4 xHCI capability probe

## Inputs Reviewed

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `sources/phoenix-rtos-usb/usb/hcd.c`
- `external/circle/include/circle/usb/xhci.h`
- `external/circle/lib/usb/xhcidevice.cpp`

## Scope Decision

- the next bounded xHCI move should be a controller-reset helper using the
  operational-register base derived from `CAPLENGTH`
- that reset slice should:
  - wait for `USBSTS.CNR` to clear before reset
  - assert `USBCMD.HCRST`
  - wait for `USBCMD.HCRST` to clear
  - verify `USBSTS.CNR` is clear again after reset
- after a successful reset, the driver should still return failure before
  enumeration, because rings, interrupts, root-hub logic, and page-size
  readiness are still missing

## Why This Comes Next

- the current xHCI code now proves controller presence and version, so the next
  missing seam is controller readiness rather than more MMIO discovery
- Circle uses exactly this reset sequence as the first controller-side step
  after version and capability checks
- it stays smaller than page-size, ring, or firmware-notify work while still
  pushing the driver toward a real runtime path

## Deferred To Later Steps

- Pi 4 firmware `PROPTAG_NOTIFY_XHCI_RESET`
- page-size validation
- command/event ring setup
- interrupt setup
- root-hub modeling and enumeration
- staging `/sbin/usb` into the live Pi 4 image
