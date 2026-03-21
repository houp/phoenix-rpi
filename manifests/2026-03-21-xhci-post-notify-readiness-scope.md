# STEP-0339: Scope the smallest post-notify xHCI readiness refinement

## Date

- 2026-03-21

## Goal

- define the smallest xHCI-internal readiness step that should follow the new
  Pi 4 firmware-notify baseline

## Inputs Reviewed

- `sources/phoenix-rtos-devices/usb/xhci/xhci.c`
- `sources/phoenix-rtos-usb/usb/hcd.c`
- `external/circle/include/circle/usb/xhci.h`
- `external/circle/lib/usb/xhcidevice.cpp`

## Scope Decision

- the next bounded xHCI move should be:
  - page-size support validation via `OP_PAGESIZE`
  - basic port-count extraction from `HCSPARAMS1`
- after that validation, the driver should still stop before root-hub,
  interrupts, rings, or enumeration

## Why This Comes Next

- Circle validates 4K page support immediately after reset and before any ring
  setup
- Phoenix already reads `HCSPARAMS1` during the capability probe, so port-count
  extraction is cheap and clarifies the later root-hub shape without widening
  into enumeration now
- this keeps the next step inside `xhci` and avoids reopening Pi 4 firmware or
  PCIe work unnecessarily

## Deferred To Later Steps

- command/event ring setup
- interrupt setup
- root-hub modeling and enumeration
- staging `/sbin/usb` into the live Pi 4 image
