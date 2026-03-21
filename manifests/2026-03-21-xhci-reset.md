# STEP-0336: Implement the first xHCI controller-reset slice

## Date

- 2026-03-21

## Repositories And SHAs

- `phoenix-rtos-devices`: `e90f12d`

## Change Summary

- extended `sources/phoenix-rtos-devices/usb/xhci/xhci.c` with the first
  controller-side readiness step after the earlier capability probe
- added:
  - operational-register access helpers based on `CAPLENGTH`
  - bounded wait helper for operational-register bits
  - controller reset sequence:
    - wait for `USBSTS.CNR` to clear
    - assert `USBCMD.HCRST`
    - wait for `USBCMD.HCRST` to clear
    - wait for `USBSTS.CNR` to clear again
- kept the driver intentionally non-operational after a successful reset:
  `xhci_init()` still returns failure before enumeration

## Validation

- refreshed the copied VM-local buildroot with:
  `./scripts/prepare-buildroot.sh --copy-components`
- built a fresh Pi 4 A72 image in `phoenix-dev` with:
  `TARGET=aarch64a72-generic-rpi4b ./phoenix-rtos-build/build.sh clean host core project image`
- result:
  build passed, and the staged Pi 4 program set remained unchanged

## Result

- the Pi 4 xHCI path is now capable of:
  - mapping MMIO
  - validating the capability header
  - executing the first bounded controller reset
- the next missing device-facing slice is now best treated separately:
  the Pi 4-specific firmware `PROPTAG_NOTIFY_XHCI_RESET` handshake before
  broader runtime USB bring-up
