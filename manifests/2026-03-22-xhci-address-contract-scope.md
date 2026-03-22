# STEP-0381: Scope the smallest xHCI address-contract step before `Address Device`

Date: `2026-03-22`

## Goal

Choose the smallest correct bridge between Phoenix USB enumeration and xHCI
slot-based addressing before implementing the first bounded `Address Device`
command path.

## Decision

For the current bounded prototype, the next seam should be:

- handle only `REQ_SET_ADDRESS` on the xHCI non-roothub control path
- require the Phoenix-requested USB address to match the xHCI slot ID returned
  by `Enable Slot`
- issue `Address Device` only under that temporary equality contract

## Rationale

- Phoenix currently allocates USB addresses in `usb/hcd.c`.
- Circle's xHCI path effectively uses slot ID as the device address after a
  successful `Address Device`.
- For the first bounded Phoenix prototype:
  - the first child device normally receives address `1`
  - the first xHCI slot is also normally `1`
- Requiring equality keeps the next implementation step narrow while making the
  temporary assumption explicit instead of silently relying on it.

## Out of scope

- generic slot-to-address translation tables
- broad multi-device xHCI address management
- generic non-roothub endpoint-0 control-transfer execution

## Validation

- source review of:
  - `phoenix-rtos-usb/usb/hcd.c`
  - `phoenix-rtos-usb/usb/dev.c`
  - `phoenix-rtos-devices/usb/xhci/xhci.c`
- reference review of:
  - `external/circle/lib/usb/xhciusbdevice.cpp`
  - `external/circle/lib/usb/xhcicommandmanager.cpp`

## Next step

Implement the bounded `REQ_SET_ADDRESS` xHCI path under the temporary
slot-ID-equals-address contract.
